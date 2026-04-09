# IoT Beverage Dispensing and Monitoring System

> **University of Louisville - CSE 596 Capstone Project (Spring 2026)**
> Industry Partner: **Multiplex Beverage**
> Developer: Karim Abdelfattah

---

## Overview

A wireless, offline-first IoT diagnostic system for commercial beverage dispensers. The system monitors drink dispense events and chiller temperature in real time using LoRa radio communication, storing all data locally on a Raspberry Pi 5 gateway and visualizing it through a Grafana dashboard — no internet required.

Built as a senior capstone for a real industry client, Multiplex Beverage, addressing the need for low-cost, infrastructure-independent monitoring at remote or low-connectivity beverage dispenser sites.

---

## System Architecture

```
[Dispenser Node]          [Chiller Node]
  Heltec LoRa32 V3          Heltec LoRa32 V3
  + Qwiic Button            + MCP9600 Thermocouple
        |                          |
        |--------  LoRa RF  ------>|
                                   |
                       [Gateway - Raspberry Pi 5]
                            MQTT Broker
                            InfluxDB (time-series DB)
                            Grafana Dashboard
```

**Dispenser Node:** A Heltec WiFi LoRa 32 V3 (ESP32-S3 + SX1262) connected to a SparkFun Qwiic Button behind the dispenser lever. Each button press-and-release represents a single drink pour. The node measures press duration, calculates estimated syrup/water/drink volume using calibrated flow rates, and transmits a structured payload over LoRa.

**Chiller Node:** A second Heltec LoRa32 V3 connected to an Adafruit MCP9600 thermocouple amplifier (K-type, I2C). Reads smoothed temperature every 5 seconds and transmits ambient + thermocouple readings over LoRa.

**Gateway:** Raspberry Pi 5 running a local MQTT broker, InfluxDB time-series database, and Grafana dashboard. All data is stored and visualized locally — no cloud dependency.

---

## Hardware

| Component | Purpose |
|---|---|
| Heltec WiFi LoRa 32 V3 (x2) | ESP32-S3 microcontroller + SX1262 LoRa radio |
| SparkFun Qwiic Button | Dispense event detection (I2C, GPIO 41/42) |
| Adafruit MCP9600 | Thermocouple amplifier for chiller temp (I2C, GPIO 41/42) |
| K-type thermocouple | Temperature probe for chiller unit |
| Raspberry Pi 5 | Gateway: MQTT + InfluxDB + Grafana |
| SSD1306 OLED 128x64 | Real-time status display on each node |

**LoRa Radio Configuration:**

| Parameter | Value |
|---|---|
| Frequency | 915 MHz (US ISM band) |
| Bandwidth | 125 kHz |
| Spreading Factor | 7 |
| Coding Rate | 4/5 |
| TX Power | 10 dBm |
| Sync Word | 0x12 |
| Preamble Length | 8 |

---

## Repository Structure

```
iot-beverage-monitoring-system/
|
|-- firmware/
|   |-- chillerTransmitter/
|   |   |-- chillerTransmitter.ino       # Production: MCP9600 temp + LoRa TX
|   |
|   |-- DispenserTransmissionTest1/
|   |   |-- DispenserTransmissionTest1.ino  # Production: button + volume calc + LoRa TX
|   |
|   |-- dev/                             # Iterative development sketches
|       |-- Dispenser_button_test1/      # I2C Qwiic Button detection
|       |-- Dispenser_button_test2/      # Press duration timing
|       |-- Dispenser_button_test3/      # Flow rate volume calculations
|       |-- HelloWorld_Sender/           # LoRa link proof-of-concept TX
|       |-- HelloWorld_Receiver/         # LoRa link proof-of-concept RX
|
|-- README.md
```

### Development Progression

The dev sketches show the full build arc:

| Sketch | Purpose |
|---|---|
| HelloWorld Sender/Receiver | Proves LoRa link between two boards before adding sensors |
| Dispenser_button_test1 | Validates Qwiic Button I2C detection on GPIO 41/42 |
| Dispenser_button_test2 | Adds press-duration timing and cumulative stats |
| Dispenser_button_test3 | Adds flow rate math: 0.5 oz/s syrup, 2.5 oz/s water, 3.0 oz/s total |
| DispenserTransmissionTest1 | Full production firmware: button + volume + LoRa TX via RadioLib |
| chillerTransmitter | Production chiller firmware: MCP9600 smoothed temp + LoRa TX via RadioLib |

---

## Payload Format

**Dispenser Node:**
```
DISPENSER,count=3,duration=1240,syrup=0.620,water=3.100,total=3.720
```

- `count` — total dispense events since boot
- `duration` — button hold time in milliseconds
- `syrup` / `water` / `total` — estimated ounces dispensed per press

**Chiller Node:**
```
CHILLER,count=7,temp=4.23,ambient=22.15
```

- `count` — total transmissions since boot
- `temp` — thermocouple reading in Celsius (5-sample average)
- `ambient` — MCP9600 onboard ambient temp in Celsius

---

## Libraries Required

Install via Arduino Library Manager:

| Library | Used In |
|---|---|
| RadioLib | Production firmware (SX1262 LoRa driver) |
| Adafruit SSD1306 | Production firmware OLED |
| Adafruit GFX | Dependency of Adafruit SSD1306 |
| Adafruit MCP9600 | chillerTransmitter |
| SparkFun Qwiic Button | DispenserTransmissionTest1 |
| HT_SSD1306Wire | Dev sketches (Heltec-specific OLED) |
| LoRaWan_APP | HelloWorld sketches only |

> Production firmware uses **RadioLib** rather than the Heltec LoRaWAN stack. This was a deliberate design decision: RadioLib provides cleaner, more portable LoRa (not LoRaWAN) control without full session handshake overhead.

---

## Board Setup

Target board: **Heltec WiFi LoRa 32 V3** (ESP32-S3 variant)

1. Install the Heltec ESP32 board package in Arduino IDE:
   `https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series`
2. Select board: `WiFi LoRa 32(V3)`
3. Upload speed: `921600`

---

## Key Design Decisions

**Offline-first architecture:** The entire stack (MQTT, InfluxDB, Grafana) runs on the Raspberry Pi 5 with no cloud dependency. Designed for venues with unreliable or no internet access.

**Dual I2C buses:** The Heltec V3 internal I2C bus is reserved for the OLED. All external sensors use a second I2C bus on GPIO 41/42 to avoid address conflicts.

**Temperature smoothing:** The chiller node averages 5 consecutive MCP9600 readings with 50 ms intervals before transmitting, reducing thermocouple noise without adding hardware.

**RadioLib migration:** Dev sketches used the Heltec LoRaWAN stack. Production firmware switched to RadioLib for simpler, more portable LoRa packet control.

---

## Project Status

- [x] LoRa link established between two nodes
- [x] Qwiic Button press detection and timing
- [x] Dispenser volume estimation from press duration
- [x] LoRa payload TX from dispenser node (RadioLib)
- [x] MCP9600 temperature reading with 5-sample smoothing
- [x] LoRa payload TX from chiller node (RadioLib)
- [ ] Raspberry Pi 5 gateway: MQTT broker + InfluxDB subscriber
- [ ] Grafana dashboard configuration
- [ ] End-to-end integration test with Multiplex Beverage hardware

---

## Author

**Karim Abdelfattah**
Dual-Degree Student, Computer and Communication Engineering
University of Louisville, J.B. Speed School of Engineering + Alexandria University

[LinkedIn](https://www.linkedin.com/in/karimabdelfattah7)
