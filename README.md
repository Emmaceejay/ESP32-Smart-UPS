# ESP32-Smart-UPS (IoT) Project

This repository contains the code and documentation for an ESP32-based Smart-UPS system integrated with a Raspberry Pi server. The project utilizes Node-RED for flow-based programming, WiFiManager for WiFi configuration, PubSubClient for MQTT messaging, and ArduinoJson for handling sensor data.

## Features

### 1. Auto Shutdown
Ensures a safe shutdown of the Raspberry Pi server during a power outage when the backup battery is low, preventing potential OS crashes.

### 2. Remote Shutdown/Boot
Allows remote shutdown or reboot of the Raspberry Pi server via MQTT over the internet, providing control from anywhere.

### 3. Backup Battery Reporting
Monitors and reports the battery status online, accessible from any location.

### 4. Status Alerts
Sends notifications via email or SMS when the battery is fully charged or critically low, ensuring timely action.

## Hardware Components

- DIY 12V battery pack
- ESP32-Wroom-32
- Raspberry Pi 3B with power supply
- Micro USB cable for ESP32 programming and power
- Buck converter (12V to 5V for ESP32, Raspberry Pi, and relays)
- Two 5V relays (5-pin)
- T-block connectors (x2)
- NPN transistors BC337 (x2)
- 10k resistors (x2)
- 100k potentiometer (POT)
- Toggle/Flip switch
- Female USB connector
- Male and female barrel connectors for charging
- Soldering iron and lead solder wire
- Flexible wires (optional)

## Software Tools

- [Node-RED](https://nodered.org/)
- [Docker](https://pimylifeup.com/raspberry-pi-docker/) for container management on Raspberry Pi
- [Mosquitto MQTT broker on Docker](https://blog.feabhas.com/2020/02/running-the-eclipse-mosquitto-mqtt-broker-in-a-docker-container/)
- Arduino IDE for programming the ESP32
- Arduino Libraries: ArduinoJson v7, WiFiManager, PubSubClient
- [ArduinoJson Web Tool](https://arduinojson.org/) for JSON serialization and deserialization

This project provides a robust solution for managing power interruptions and remotely controlling a Raspberry Pi server, making it ideal for IoT applications.
