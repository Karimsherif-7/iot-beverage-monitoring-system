// Dev sketch 3/3 - Flow rate volume estimation
// Goal: convert press duration to syrup/water/drink volume estimates
// Flow rates provided by Multiplex Beverage:
//   Syrup = 0.5 oz/sec | Water = 2.5 oz/sec | Total = 3.0 oz/sec

#include "Arduino.h"
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include <SparkFun_Qwiic_Button.h>

SSD1306Wire factory_display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
TwoWire I2C_Button = TwoWire(1);
QwiicButton button;

bool buttonFound  = false;
bool lastPressed  = false;
unsigned long pressStartTime = 0;
unsigned long pressDuration  = 0;
unsigned long totalPressTime = 0;
unsigned long pressCount     = 0;

const float SYRUP_RATE_PER_MS = 0.0005f;  // 0.5 oz/sec
const float WATER_RATE_PER_MS = 0.0025f;  // 2.5 oz/sec
const float DRINK_RATE_PER_MS = 0.0030f;  // 3.0 oz/sec

float syrupUsedLastPress = 0.0f;
float waterUsedLastPress = 0.0f;
float drinkUsedLastPress = 0.0f;
float totalSyrupUsed = 0.0f;
float totalWaterUsed = 0.0f;
float totalDrinkUsed = 0.0f;

void VextON()  { pinMode(Vext, OUTPUT); digitalWrite(Vext, LOW);  }
void VextOFF() { pinMode(Vext, OUTPUT); digitalWrite(Vext, HIGH); }

void showStatus(const String &l1, const String &l2 = "",
                const String &l3 = "", const String &l4 = "") {
  factory_display.clear();
  factory_display.drawString(0,  0, l1);
  if (l2.length()) factory_display.drawString(0, 16, l2);
  if (l3.length()) factory_display.drawString(0, 32, l3);
  if (l4.length()) factory_display.drawString(0, 48, l4);
  factory_display.display();
}

void setup() {
  Serial.begin(115200);
  delay(500);
  VextON();
  delay(100);
  factory_display.init();
  showStatus("Heltec V3 Boot", "OLED OK", "Init I2C");

  I2C_Button.begin(4, 2);
  I2C_Button.setTimeOut(50);
  delay(100);

  if (button.begin(0x6F, I2C_Button) == false) {
    buttonFound = false;
    showStatus("Qwiic Button", "NOT FOUND");
  } else {
    buttonFound = true;
    showStatus("Qwiic Button", "Connected", "Drink=3.0 oz/s", "Waiting...");
    delay(1000);
  }
}

void loop() {
  if (!buttonFound) { delay(500); return; }

  bool pressed = button.isPressed();

  if (pressed && !lastPressed) {
    pressStartTime = millis();
    showStatus("BUTTON PRESSED", "Timing...",
               "Count: " + String(pressCount + 1),
               "TotalDrink: " + String(totalDrinkUsed, 2) + " oz");
  }

  if (!pressed && lastPressed) {
    pressDuration   = millis() - pressStartTime;
    totalPressTime += pressDuration;
    pressCount++;

    syrupUsedLastPress = pressDuration * SYRUP_RATE_PER_MS;
    waterUsedLastPress = pressDuration * WATER_RATE_PER_MS;
    drinkUsedLastPress = pressDuration * DRINK_RATE_PER_MS;

    totalSyrupUsed += syrupUsedLastPress;
    totalWaterUsed += waterUsedLastPress;
    totalDrinkUsed += drinkUsedLastPress;

    Serial.printf("Press #%lu | %lu ms | Drink: %.3f oz | Total: %.3f oz\n",
                  pressCount, pressDuration, drinkUsedLastPress, totalDrinkUsed);

    showStatus("BUTTON RELEASED",
               "Last: " + String(drinkUsedLastPress, 2) + " oz",
               "Total: " + String(totalDrinkUsed, 2) + " oz",
               "Count: " + String(pressCount));
  }

  lastPressed = pressed;
  delay(10);
}
