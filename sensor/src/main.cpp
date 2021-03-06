#ifdef TEMPERATURE_PORT
#include <DHT.h>
#endif
#ifdef SOUND_PORT
#include "sound.h"
#endif

#include <ArduinoJson.h>
#include "wifi.h"
#include "mDns.h"
#include "ota.h"

const char* hostname = HOSTNAME;
const char* location = LOCATION;

// TCP server at port 80 will respond to HTTP requests
WiFiServer server(80);

#ifdef TEMPERATURE_PORT
DHT dht;
#endif

void waitForClient(WiFiClient* client) {
    // Wait for data from client to become available
    while (client->connected() && !client->available()) {
        delay(1);
    }
}

void setup() {
    Serial.begin(115200);

    initWifi();
    initOta(hostname);
    initMDns(hostname);

    #ifdef TEMPERATURE_PORT
        dht.setup(TEMPERATURE_PORT);
    #endif
    #ifdef SOUND_PORT
        initSound();
    #endif

    // Start TCP (HTTP) server
    server.begin();
    Serial.println("TCP server started");

    // Add service to MDNS-SD
    MDNS.addService("temperature", "tcp", 80);
}

void loop() {
    ArduinoOTA.handle();

    #ifdef SOUND_PORT
        handleSound();
    #endif

    // Check if a client has connected
    WiFiClient client = server.available();
    if (!client) {
        return;
    }
    Serial.println("");
    Serial.println("New client");

    waitForClient(&client);

    // Read the first line of HTTP request
    String req = client.readStringUntil('\r');

    // First line of HTTP request looks like "GET /path HTTP/1.1"
    // Retrieve the "/path" part by finding the spaces
    int addr_start = req.indexOf(' ');
    int addr_end = req.indexOf(' ', addr_start + 1);
    if (addr_start == -1 || addr_end == -1) {
        Serial.print("Invalid request: ");
        Serial.println(req);
        return;
    }
    req = req.substring(addr_start + 1, addr_end);
    Serial.print("Request: ");
    Serial.println(req);
    client.flush();

    String s;
    if (req == "/") {
        StaticJsonBuffer<500> jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        JsonObject& sensors = root.createNestedObject("sensors");
        JsonObject& labels = root.createNestedObject("labels");


        #ifdef TEMPERATURE_PORT
        sensors[F("temperature")] = dht.getTemperature();
        sensors[F("humidity")] = dht.getHumidity();
        #endif

        #ifdef SOUND_PORT
        sensors[F("sound")] = getSoundDiff();
        #endif

        labels[F("location")] = location;

        // Write response headers
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println();
        root.prettyPrintTo(client);


        Serial.println("Sending 200");
    } else {
        Serial.println("Sending 404");
        client.println("HTTP/1.1 404 Not Found");
        client.println();
    }

    Serial.println("Done with client");
}

