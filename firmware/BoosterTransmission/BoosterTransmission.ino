#include <RadioLib.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BME280.h>

#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21

#define SDA_PIN               41
#define SCL_PIN               42
#define CURRENT_PIN           20

TwoWire I2Ctwo = TwoWire(1);
Adafruit_BME280 bme;
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RST);

SX1262 radio = new Module(8, 14, 12, 13);

const float FREQUENCY = 915.0;
const float BANDWIDTH = 125.0;
const uint8_t SPREAD_FACTOR = 7;
const uint8_t CODING_RATE = 5;
const uint8_t SYNC_WORD = 0x12;
const int8_t TX_POWER = 10;
const uint16_t PREAMBLE_LEN = 8;

unsigned long packetCount = 0;
unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL = 30000;

int current;
float temp;
float pressure;
float power;

void setup() {
  Serial.begin(115200);

  pinMode(CURRENT_PIN, INPUT);
  I2Ctwo.begin(SDA_PIN, SCL_PIN);
  bme.begin(0x77, &I2Ctwo);

  delay(2000);
  pinMode(36, OUTPUT);
  digitalWrite(36, LOW);
  delay(100);
  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Booster Node");
  display.display();

  int state = radio.begin(FREQUENCY, BANDWIDTH, SPREAD_FACTOR, CODING_RATE, SYNC_WORD, TX_POWER, PREAMBLE_LEN);

  if (state != RADIOLIB_ERR_NONE) {
    display.println("Radio FAILED");
    display.display();
    while (true) { delay(1000); }
  }

  radio.setCRC(0);
  radio.setDio2AsRfSwitch(true);

  display.println("Radio OK");
  display.println("Waiting...");
  display.display();
}

float getCurrentReading() {
  int adc = analogRead(CURRENT_PIN);
  float vout = (adc / 4095.0) * 3.3;
  Serial.println(vout);
  float current = (vout - 2.385) / 0.185;
  return current;
}

float testCurrentZero() {
  int zeroADC = analogRead(CURRENT_PIN);
  float zeroVoltage = (zeroADC / 4095.0) * 3.3;
  Serial.println(zeroVoltage);
  return (zeroVoltage);
}


float getPowerReading() {
  float current = getCurrentReading();
  float power = current * 12.0;
  return power;
}

float getTemperatureReading() {
  float temp = bme.readTemperature();
  return temp;
}

float getPressureReading() {
  float pressure = bme.readPressure();
  return pressure;
}

void loop() {
  if (millis() - lastSend >= SEND_INTERVAL) {
    lastSend = millis();
    packetCount++;

    String payload = "BOOSTER,count=" + String(packetCount)
                   + ",current=" + String(getCurrentReading())
                   + ",power=" + String(getPowerReading())
                   + ",temp=" + String(getTemperatureReading())
                   + ",pressure=" + String(getPressureReading());

    int state = radio.transmit(payload);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("C: ");
    display.print(getCurrentReading());
    display.print(" Pwr: ");
    display.println(getPowerReading());
    display.print("T: ");
    display.print(getTemperatureReading());
    display.print(" P: ");
    display.println(getPressureReading());
    display.println();
    display.println(state == RADIOLIB_ERR_NONE ? "Transmission sent" : "Transmission failed");
    display.display();

    Serial.print("TX: ");
    Serial.println(payload);
  }
}