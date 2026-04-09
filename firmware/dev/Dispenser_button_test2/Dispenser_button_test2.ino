// Dev sketch 2/3 - Button press duration timing
// Goal: accurately measure how long the button is held (= pour duration)

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
  showStatus("Heltec V3 Boot", "OLED OK", "Init external I2C");

  I2C_Button.begin(41, 42);
  I2C_Button.setTimeOut(50);
  delay(100);

  if (button.begin(0x6F, I2C_Button) == false) {
    buttonFound = false;
    showStatus("Qwiic Button", "NOT FOUND");
  } else {
    buttonFound = true;
    showStatus("Qwiic Button", "Connected", "Addr: 0x6F", "Waiting...");
    delay(1000);
  }
}

void loop() {
  if (!buttonFound) { delay(500); return; }

  bool pressed = button.isPressed();

  if (pressed && !lastPressed) {
    pressStartTime = millis();
    showStatus("BUTTON PRESSED", "Timing...",
               "Next #: " + String(pressCount + 1),
               "Total: " + String(totalPressTime) + " ms");
  }

  if (!pressed && lastPressed) {
    pressDuration   = millis() - pressStartTime;
    totalPressTime += pressDuration;
    pressCount++;

    Serial.printf("Press #%lu: %lu ms | Total: %lu ms\n",
                  pressCount, pressDuration, totalPressTime);

    showStatus("BUTTON RELEASED",
               "Last: "  + String(pressDuration)  + " ms",
               "Total: " + String(totalPressTime) + " ms",
               "Count: " + String(pressCount));
  }

  lastPressed = pressed;
  delay(10);
}
