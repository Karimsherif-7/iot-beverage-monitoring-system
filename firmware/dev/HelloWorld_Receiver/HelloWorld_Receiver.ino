// LoRa link proof-of-concept - Receiver
// Purpose: verify LoRa radio RX from HelloWorld_Sender
// Note: uses Heltec LoRaWAN stack. Production firmware migrated to RadioLib.

#include "Arduino.h"
#include "LoRaWan_APP.h"
#include <Wire.h>
#include "HT_SSD1306Wire.h"

#define RF_FREQUENCY              868000000
#define LORA_BANDWIDTH            0
#define LORA_SPREADING_FACTOR     7
#define LORA_CODINGRATE           1
#define LORA_PREAMBLE_LENGTH      8
#define LORA_SYMBOL_TIMEOUT       0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON      false
#define BUFFER_SIZE               128

static RadioEvents_t RadioEvents;
char rxpacket[BUFFER_SIZE];
volatile bool receiveflag = false;
int16_t Rssi   = 0;
int8_t  Snr    = 0;
uint16_t rxSize = 0;

SSD1306Wire factory_display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

void VextON()  { pinMode(Vext, OUTPUT); digitalWrite(Vext, LOW);  }
void VextOFF() { pinMode(Vext, OUTPUT); digitalWrite(Vext, HIGH); }

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  Rssi = rssi; Snr = snr; rxSize = size;
  if (size >= BUFFER_SIZE) size = BUFFER_SIZE - 1;
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';
  receiveflag = true;
  Radio.Sleep();
}

void lora_init_rx() {
  Mcu.begin();
  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);
  Radio.Rx(0);
}

void setup() {
  Serial.begin(115200);
  VextON();
  delay(50);
  factory_display.init();
  factory_display.clear();
  factory_display.drawString(0,  0, "LoRa Receiver");
  factory_display.drawString(0, 15, "Waiting...");
  factory_display.display();
  lora_init_rx();
}

void loop() {
  Radio.IrqProcess();
  if (receiveflag) {
    receiveflag = false;
    factory_display.clear();
    factory_display.drawString(0,  0, "LoRa Receiver");
    factory_display.drawString(0, 15, "RX: " + String(rxpacket));
    factory_display.drawString(0, 45, "RSSI:" + String(Rssi) + " SNR:" + String(Snr));
    factory_display.display();
    Radio.Rx(0);
  }
}
