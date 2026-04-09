// LoRa link proof-of-concept - Sender
// Purpose: verify LoRa radio TX between two Heltec V3 boards before adding sensors
// Note: uses Heltec LoRaWAN stack. Production firmware migrated to RadioLib.

#include "Arduino.h"
#include "LoRaWan_APP.h"
#include <Wire.h>
#include "HT_SSD1306Wire.h"

#define RF_FREQUENCY              868000000
#define TX_OUTPUT_POWER           10
#define LORA_BANDWIDTH            0
#define LORA_SPREADING_FACTOR     7
#define LORA_CODINGRATE           1
#define LORA_PREAMBLE_LENGTH      8
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON      false
#define BUFFER_SIZE               64
#define TX_INTERVAL_MS            2000

static RadioEvents_t RadioEvents;
char txpacket[BUFFER_SIZE];
volatile bool txDoneFlag    = false;
volatile bool txTimeoutFlag = false;
uint16_t txNumber = 0;

SSD1306Wire factory_display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

void VextON()  { pinMode(Vext, OUTPUT); digitalWrite(Vext, LOW);  }
void VextOFF() { pinMode(Vext, OUTPUT); digitalWrite(Vext, HIGH); }
void OnTxDone()    { txDoneFlag    = true; }
void OnTxTimeout() { txTimeoutFlag = true; }

void lora_init_tx() {
  Mcu.begin();
  RadioEvents.TxDone    = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);
}

void setup() {
  Serial.begin(115200);
  VextON();
  delay(50);
  factory_display.init();
  factory_display.clear();
  factory_display.drawString(0,  0, "LoRa Sender");
  factory_display.drawString(0, 15, "Starting...");
  factory_display.display();
  lora_init_tx();
}

void loop() {
  txNumber++;
  snprintf(txpacket, BUFFER_SIZE, "Hello from LoRa32 #%u", txNumber);
  txDoneFlag = txTimeoutFlag = false;

  factory_display.clear();
  factory_display.drawString(0,  0, "LoRa Sender");
  factory_display.drawString(0, 15, "TX: " + String(txpacket));
  factory_display.drawString(0, 50, "Sending...");
  factory_display.display();

  Radio.Send((uint8_t *)txpacket, strlen(txpacket));

  unsigned long start = millis();
  while (!txDoneFlag && !txTimeoutFlag) {
    Radio.IrqProcess();
    if (millis() - start > 5000) break;
  }

  factory_display.clear();
  factory_display.drawString(0,  0, "LoRa Sender");
  factory_display.drawString(0, 15, String(txpacket));
  factory_display.drawString(0, 50,
    txDoneFlag ? "TX done" : (txTimeoutFlag ? "TX timeout" : "TX unknown"));
  factory_display.display();

  delay(TX_INTERVAL_MS);
}
