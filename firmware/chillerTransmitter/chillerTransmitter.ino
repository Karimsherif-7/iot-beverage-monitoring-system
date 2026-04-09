#include <RadioLib.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MCP9600.h>

// ---------------- OLED ----------------
#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RST);

// ---------------- External I2C for MCP9600 ----------------
// Uses second I2C bus on GPIO 41/42 to avoid conflict with OLED
TwoWire I2C_Temp = TwoWire(1);
Adafruit_MCP9600 mcp;

// ---------------- LoRa Radio ----------------
SX1262 radio = new Module(8, 14, 12, 13);

const float    FREQUENCY    = 915.0;  // MHz - US ISM band
const float    BANDWIDTH    = 125.0;  // kHz
const uint8_t  SPREAD_FACTOR = 7;
const uint8_t  CODING_RATE   = 5;
const uint8_t  SYNC_WORD     = 0x12;
const int8_t   TX_POWER      = 10;   // dBm
const uint16_t PREAMBLE_LEN  = 8;

// ---------------- Chiller config ----------------
bool sensorFound = false;
unsigned long packetCount = 0;
unsigned long lastSend    = 0;
const unsigned long SEND_INTERVAL = 5000; // ms between transmissions

const float TEMP_OFFSET = 0.0; // calibration offset in Celsius
const int   NUM_SAMPLES = 5;   // readings averaged per transmission

// ---------------- OLED helper ----------------
void showLines(String l1, String l2 = "", String l3 = "", String l4 = "") {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(l1);
  if (l2.length()) display.println(l2);
  if (l3.length()) display.println(l3);
  if (l4.length()) display.println(l4);
  display.display();
}

// ---------------- Smoothed temperature reading ----------------
float readSmoothedTemp() {
  float sum = 0.0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sum += mcp.readThermocouple();
    delay(50);
  }
  return (sum / NUM_SAMPLES) + TEMP_OFFSET;
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  // Power on Heltec OLED power rail
  pinMode(36, OUTPUT);
  digitalWrite(36, LOW);
  delay(100);

  // OLED on internal I2C
  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  showLines("Chiller Node", "Booting...");

  // External I2C bus for MCP9600 (slow clock for stability)
  I2C_Temp.begin(41, 42, 1000);
  I2C_Temp.setTimeOut(50);
  delay(500);

  // MCP9600 thermocouple amplifier init
  if (!mcp.begin(0x60, &I2C_Temp)) {
    sensorFound = false;
    showLines("Chiller Node", "MCP9600", "NOT FOUND");
    while (true) delay(1000);
  }

  sensorFound = true;
  mcp.setADCresolution(MCP9600_ADCRESOLUTION_18);
  mcp.setThermocoupleType(MCP9600_TYPE_K);
  mcp.setFilterCoefficient(3);
  mcp.enable(true);
  showLines("Chiller Node", "MCP9600 OK");
  delay(1000);

  // LoRa radio init via RadioLib
  int state = radio.begin(FREQUENCY, BANDWIDTH, SPREAD_FACTOR,
                          CODING_RATE, SYNC_WORD, TX_POWER, PREAMBLE_LEN);
  if (state != RADIOLIB_ERR_NONE) {
    showLines("Chiller Node", "Radio FAILED", "Code: " + String(state));
    while (true) delay(1000);
  }

  radio.setCRC(0);
  radio.setDio2AsRfSwitch(true);
  showLines("Chiller Node", "Radio OK", "Transmitting...");
}

void loop() {
  if (!sensorFound) { delay(100); return; }

  if (millis() - lastSend >= SEND_INTERVAL) {
    lastSend = millis();
    packetCount++;

    float ambientC = mcp.readAmbient();
    float tempC    = readSmoothedTemp();

    // Payload format: CHILLER,count=<n>,temp=<C>,ambient=<C>
    String payload = "CHILLER,count=" + String(packetCount)
                   + ",temp="    + String(tempC, 2)
                   + ",ambient=" + String(ambientC, 2);

    int state = radio.transmit(payload);

    Serial.println("TX: " + payload);
    showLines(
      "Chiller Node",
      (state == RADIOLIB_ERR_NONE) ? "Sent OK" : "TX Failed",
      "Temp: "    + String(tempC, 2)    + " C",
      "Ambient: " + String(ambientC, 2) + " C"
    );
  }

  delay(10);
}
