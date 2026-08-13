# Smart Fabric Bioluminescence System

This project implements an interactive textile lighting system that uses conductive yarn and MPR121 capacitive touch sensors to detect user interaction and map it to a bioluminescent LED display.

## System Overview

The system combines:
- conductive textile input
- MPR121 capacitive sensing
- TCA9548A multiplexing
- Arduino-based control logic
- NeoPixel LED output
- optional WiFi-based mode switching

## PlantUML Diagram

The diagram below shows the main system structure and relationships between hardware and software components.

```plantuml
@startuml
left to right direction
skinparam componentStyle rectangle

actor User
rectangle "Interactive Fabric" as Fabric
rectangle "MPR121 Touch Sensors" as MPR
rectangle "TCA9548A Multiplexer" as Mux
rectangle "Arduino Controller" as Arduino
rectangle "Lighting Modes" as Modes
rectangle "NeoPixel LED Panels" as LEDs
rectangle "WiFi Control" as Wifi

User --> Fabric : touches
Fabric --> MPR : conductive yarn signal
MPR --> Mux : sensor data
Mux --> Arduino : selected channel data
Arduino --> Modes : mode selection
Modes --> Arduino : active lighting behavior
Arduino --> LEDs : visual output
Arduino --> Wifi : wireless mode switching
Wifi --> Arduino : remote command

@enduml
```

## Project Structure

- `BioLuminiscence/BioDecay/bioluminescence/` contains the Arduino project files
- `ProjectConfig.h` defines hardware mapping and lighting geometry
- `bioluminescence.ino` contains the main runtime logic
- `WiFiController.h` handles remote mode switching
- `ModeController` and sensor classes manage the interaction system

## Purpose

The system turns a soft fabric surface into an interactive input medium. Touching the conductive yarn changes capacitance, which is detected by the MPR121 sensors and translated into dynamic lighting effects on the LED panels.

## Notes

This project is intended for prototyping and experimentation with interactive textile interfaces and responsive bioluminescent output.
