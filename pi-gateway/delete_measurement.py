#!/usr/bin/env python3
"""
delete_measurement.py
Utility script to delete all data from a specific InfluxDB measurement.
Useful for clearing test data during development.

WARNING: Deletes ALL data in the specified measurement across all time.
Edit the 'predicate' field to target a different measurement before running.

IMPORTANT: Set your InfluxDB token in the environment variable INFLUX_TOKEN
before running.

  export INFLUX_TOKEN="your_token_here"
  python3 delete_measurement.py
"""
import os
import requests

INFLUX_DELETE_URL = "https://us-east-1-1.aws.cloud2.influxdata.com/api/v2/delete"
INFLUX_TOKEN      = os.environ.get("INFLUX_TOKEN", "***INFLUX_TOKEN_NOT_SET***")
INFLUX_ORG        = "Shepherds"
INFLUX_BUCKET     = "Multiplex_Sensor_Data"

params = {"org": INFLUX_ORG, "bucket": INFLUX_BUCKET}
headers = {
    "Authorization": f"Token {INFLUX_TOKEN}",
    "Content-Type": "application/json"
}

# Edit predicate to target the measurement you want to delete
data = {
    "start": "1970-01-01T00:00:00Z",
    "stop":  "2026-12-31T00:00:00Z",
    "predicate": '_measurement="dispenser_table"'
}

response = requests.post(INFLUX_DELETE_URL, headers=headers, params=params, json=data)
print(response.status_code)
print(response.text)
