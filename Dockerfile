FROM python:3-slim-stretch

COPY main.py /root/main.py

RUN apt update && apt upgrade -y && apt install -y build-essential && pip3 install Adafruit_Python_DHT

EXPOSE 8000

ENTRYPOINT /root/main.py


