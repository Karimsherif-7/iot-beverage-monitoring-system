# IoT Beverage Dispensing and Monitoring System

> **University of Louisville — CSE 596 Capstone Project (Spring 2026)**

---

## Overview

A wireless, offline-first IoT diagnostic platform for commercial beverage dispensers. Three sensor nodes transmit real-time telemetry over LoRa radio to a Raspberry Pi 5 gateway, where data is stored in InfluxDB and visualized through a Grafana dashboard — no internet or Wi-Fi required at the venue.

Built as a senior capstone for a real industry client, Multiplex Beverage, addressing the lack of real-time visibility into machine health across restaurant, stadium, and retail deployments.

---

## How It Started — Proving the Radio Link

Before writing any sensor code, the first milestone was confirming that two **Heltec WiFi LoRa 32 V3** boards could actually talk to each other over LoRa.

This was done using the factory pingpong example from Heltec, then split into dedicated sender and receiver sketches. One board transmitted a `HelloWorld_<seq>` packet every few seconds; the other received it and displayed the message on its onboard OLED along with RSSI and SNR. Once both OLEDs were showing the correct packets in real time, the radio link was confirmed.

Those sketches live in `firmware/dev/HelloWorld_Sender/` and `firmware/dev/HelloWorld_Receiver/`. They were the foundation everything else was built on.

---

## System Architecture

```
[Dispenser Node]     [Chiller Node]     [Booster Node]
 Heltec LoRa32 V3    Heltec LoRa32 V3   Heltec LoRa32 V3
 + Qwiic Button      + MCP9600 TC AMP   + ACS723 + DC Motor
       |                   |                   |
       +------- LoRa RF (915 MHz) ------------+
                               |
                  [Raspberry Pi 5 Gateway]
                       SX1262 LoRa HAT
                       Python receiver
                       InfluxDB (time-series)
                       Grafana Dashboard
                       HDMI local display
```

All communication is **star topology** — every node transmits directly to the Pi 5. The Pi runs the entire stack locally with no cloud dependency.

---

## Hardware

| Component | Role |
|---|---|
| Heltec WiFi LoRa 32 V3 (×3) | ESP32-S3 + SX1262 radio, one per node |
| SparkFun Qwiic Button | Dispense event detection (Dispenser Node) |
| Adafruit MCP9600 thermocouple amp | Temperature sensing (Chiller Node) |
| K-type thermocouple | Physical temperature probe |
| ACS723 current sensor | Pump current monitoring (Booster Node) |
| DC motor | Simulated pump (Booster Node) |
| Raspberry Pi 5 | Gateway: receives, stores, and displays all telemetry |
| SX1262 LoRa HAT | Radio interface on the Pi 5 |

**LoRa Radio Config (all nodes + gateway must match):**

| Parameter | Value |
|---|---|
| Frequency | 915 MHz |
| Bandwidth | 125 kHz |
| Spreading Factor | 7 |
| Coding Rate | 4/5 |
| Sync Word | 0x12 |
| Preamble | 8 |

---

## Repository Structure

```
iot-beverage-monitoring-system/
|
|-- firmware/                         # Arduino sketches for all three nodes
|   |-- chillerTransmitter/
|   |   |-- chillerTransmitter.ino    # Chiller Node: MCP9600 temp + LoRa TX
|   |
|   |-- DispenserTransmissionTest1/
|   |   |-- DispenserTransmissionTest1.ino  # Dispenser Node: button + volume + LoRa TX
|   |
|   |-- dev/                          # Development and test sketches
|       |-- HelloWorld_Sender/        # LoRa pingpong TX (first proof of radio link)
|       |-- HelloWorld_Receiver/      # LoRa pingpong RX
|       |-- Dispenser_button_test1/   # Qwiic Button I2C detection on GPIO 41/42
|       |-- Dispenser_button_test2/   # Press duration timing
|       |-- Dispenser_button_test3/   # Flow rate volume calculations
|
|-- pi-gateway/                       # Python scripts running on the Raspberry Pi 5
|   |-- chillerReceiver.py            # Bare-bones LoRa receiver, prints to stdout
|   |-- LoRaReceiver_InfluxSender.py  # Receiver + InfluxDB writer (v1)
|   |-- LoRaReceiver_InfluxSender_Updated.py  # Receiver + InfluxDB writer (v2, active)
|   |-- delete_measurement.py         # Dev utility: wipe a measurement from InfluxDB
|   |-- README.md                     # Pi gateway setup and usage details
|
|-- README.md                         # This file
```

---

## Firmware — Node Sketches Explained

### `firmware/dev/HelloWorld_Sender` and `HelloWorld_Receiver`

The starting point. These sketches use the Heltec LoRaWAN stack to send and receive a simple `HelloWorld_<seq>` string. The sender increments a counter each transmission; the receiver displays the packet on its OLED with RSSI and SNR. Used to confirm the radio link was working between two boards before any sensor code was added.

### `firmware/dev/Dispenser_button_test1`

Focused on getting the **SparkFun Qwiic Button** talking to the Heltec V3. Because the V3's OLED uses the internal I2C bus, a second I2C bus was created on GPIO 41 (SDA) and GPIO 42 (SCL) using `TwoWire(1)`. This sketch confirms the button appears on that bus at address `0x6F` and counts presses.

### `firmware/dev/Dispenser_button_test2`

Adds timing. On each button press, `millis()` records the start time. On release, press duration is computed and accumulated. Output shows per-press duration and total cumulative press time.

### `firmware/dev/Dispenser_button_test3`

Adds volume calculations using the flow rate constants provided by Multiplex Beverage:
- Syrup: 0.5 oz/sec (0.0005 oz/ms)
- Water: 2.5 oz/sec (0.0025 oz/ms)
- Total drink: 3.0 oz/sec (0.0030 oz/ms)

Each press duration is converted to ounces of syrup, water, and total drink dispensed. Cumulative totals are tracked across all presses.

### `firmware/DispenserTransmissionTest1/DispenserTransmissionTest1.ino`

Production dispenser firmware. Combines the button timing and volume math from the dev sketches, then transmits a structured LoRa payload using **RadioLib** on button release. The packet is prefixed with `D:` so the Pi gateway can identify it as a dispenser packet.

Example transmitted packet:
```
D:syrup=0.620,water=3.100,total=3.720,count=3,duration=1240
```

### `firmware/chillerTransmitter/chillerTransmitter.ino`

Production chiller firmware. Reads from an **Adafruit MCP9600** thermocouple amplifier over I2C (same GPIO 41/42 second bus). Takes a 5-sample average with 50 ms spacing to smooth thermocouple noise, then transmits over LoRa using RadioLib. Prefixed with `C:`.

Example transmitted packet:
```
C:temp=4.23,ambient=22.15,count=7
```

> **Note on RadioLib vs Heltec stack:** The dev sketches use the Heltec LoRaWAN library (`LoRaWan_APP.h`). Production firmware migrated to **RadioLib** because it gives direct LoRa packet control without the overhead or session requirements of LoRaWAN. The Pi gateway also talks raw LoRa, not LoRaWAN, which is why they are compatible.

---

## Pi Gateway — Scripts Explained

All Pi gateway scripts live in `pi-gateway/`. They communicate with the SX1262 LoRa HAT via direct SPI using `spidev` and `lgpio`.

### `pi-gateway/chillerReceiver.py`

The first script used on the Pi after pingpong testing was confirmed. It does one thing: listen for LoRa packets and print them to stdout. No database, no routing — just raw packet reception. Used to verify the Pi's radio HAT was working before adding any data pipeline.

### `pi-gateway/LoRaReceiver_InfluxSender.py` (v1)

Builds on `chillerReceiver.py` by adding InfluxDB writes. When a packet arrives, it reads the first character to determine which node sent it:
- `C` → writes to `chiller_table`
- `B` → writes to `booster_table`
- `D` → writes to `dispenser_table`

In this version the whole payload is stored as a single raw string `value` field. This works but makes it harder for Grafana to plot individual metrics without string parsing.

### `pi-gateway/LoRaReceiver_InfluxSender_Updated.py` (v2 — active)

The version currently in use. Instead of wrapping the payload in a string field, it passes the payload directly as InfluxDB line protocol field syntax. This means Grafana sees each metric (`syrup`, `water`, `temp`, etc.) as a proper individual field it can query and graph natively.

It also adds syrup remaining tracking for the Dispenser Node: after each dispense event, it queries InfluxDB for the last recorded syrup dispensed and last syrupRemaining value, computes the new remaining volume, and appends it to the packet before writing.

Measurements written:
- `chiller_node_table`
- `booster_node_table`
- `dispenser_node_table_w_syrupRemaining`

### `pi-gateway/delete_measurement.py`

A one-shot utility that sends a delete request to InfluxDB to wipe all records from a specific measurement. Used during development to clear test data between runs. Edit the `predicate` field before running to target the right measurement.

---

## Running the Gateway

```bash
# Set your InfluxDB token as an environment variable (never hardcode it)
export INFLUX_TOKEN="your_influxdb_token_here"

# Activate the virtual environment
source /path/to/CapstoneEnv/bin/activate

# Run the active receiver
python3 pi-gateway/LoRaReceiver_InfluxSender_Updated.py
```

---

## Payload Format

**Dispenser Node** (prefix `D:`):
```
D:syrup=0.620,water=3.100,total=3.720,count=3,duration=1240
```

**Chiller Node** (prefix `C:`):
```
C:temp=4.23,ambient=22.15,count=7
```

**Booster Node** (prefix `B:`):
```
B:current=1.82,state=ON,temp=23.4,seq=104
```

---

## Libraries Required

### Arduino (install via Library Manager)

| Library | Used In |
|---|---|
| RadioLib | Production firmware (SX1262 LoRa driver) |
| Adafruit SSD1306 | OLED display on production nodes |
| Adafruit GFX | Dependency of Adafruit SSD1306 |
| Adafruit MCP9600 | chillerTransmitter |
| SparkFun Qwiic Button | DispenserTransmissionTest1 |
| HT_SSD1306Wire | Dev sketches (Heltec internal OLED) |
| LoRaWan_APP | HelloWorld sketches only |

### Python (Pi gateway)

```bash
pip install spidev lgpio requests
```

---

## Board Setup (Arduino IDE)

Target board: **Heltec WiFi LoRa 32 V3**

1. Add Heltec board package URL in Arduino IDE preferences:
   `https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series`
2. Install **Heltec ESP32 Series Dev-boards** from Boards Manager
3. Select board: `WiFi LoRa 32(V3)`
4. Upload speed: `921600`

---

## Project Status

- [x] LoRa pingpong link confirmed between two nodes
- [x] Qwiic Button I2C detection on external bus (GPIO 41/42)
- [x] Press duration timing and cumulative tracking
- [x] Dispenser volume estimation from press duration + flow rates
- [x] LoRa TX from Dispenser Node (RadioLib)
- [x] MCP9600 temperature reading with 5-sample smoothing
- [x] LoRa TX from Chiller Node (RadioLib)
- [x] Pi 5 bare receiver working (chillerReceiver.py)
- [x] Pi 5 InfluxDB writer working (LoRaReceiver_InfluxSender_Updated.py)
- [ ] Grafana dashboard panels configured
- [ ] Booster Node firmware (ACS723 current sensing)
- [ ] End-to-end integration test with Multiplex Beverage hardware

---

## Author

**Karim Abdelfattah**
Dual-Degree Student, Computer and Communication Engineering
University of Louisville, J.B. Speed School of Engineering + Alexandria University

[LinkedIn](https://www.linkedin.com/in/karimabdelfattah7)
