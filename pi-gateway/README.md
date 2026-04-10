# Pi 5 Gateway Scripts

This folder contains all Python scripts running on the Raspberry Pi 5 gateway for the Multiplex Beverage Health and Diagnostic Dashboard.

## Hardware
- Raspberry Pi 5 (gateway + dashboard host)
- SX1262 LoRa HAT connected via SPI (pins: CS=21, BUSY=24, RST=22, DIO1=16, TXEN=6)
- Radio config: 915 MHz, BW=125 kHz, SF=7, CR=5

## Files

### `chillerReceiver.py`
Bare-bones LoRa receiver. Prints all received packets to stdout. No InfluxDB integration. Use this for testing radio reception before enabling data logging.

### `LoRaReceiver_InfluxSender.py` (v1)
LoRa receiver that routes packets to separate InfluxDB measurements by node prefix:
- `C` prefix → `chiller_table`
- `B` prefix → `booster_table`
- `D` prefix → `dispenser_table`

Stores the raw payload string as a `value` field.

### `LoRaReceiver_InfluxSender_Updated.py` (v2 - active)
Updated receiver that writes InfluxDB line protocol field strings directly, enabling Grafana to read individual fields natively. Also adds real-time syrup remaining calculation for the Dispenser Node by querying the last known values from InfluxDB.

Measurements written:
- `chiller_node_table`
- `booster_node_table`
- `dispenser_node_table_w_syrupRemaining`

### `delete_measurement.py`
Utility to delete all records from a specified InfluxDB measurement. Edit the `predicate` field before running.

## InfluxDB Configuration
- **Cloud region:** AWS us-east-1
- **Org:** Shepherds
- **Bucket:** Multiplex_Sensor_Data

## Running
```bash
# Activate the virtual environment first
source /path/to/CapstoneEnv/bin/activate

# Run the active receiver
python3 LoRaReceiver_InfluxSender_Updated.py
```

## Node Packet Format
Nodes prefix their packets with a single character to identify the source:
- `C:` = Chiller Node
- `B:` = Booster Node
- `D:` = Dispenser Node

The payload after the prefix is a comma-separated InfluxDB field string, e.g.:
```
D:syrup=1.44,water=7.19,drink=8.63,count=3,duration=2875
```
