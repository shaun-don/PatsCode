#include <M5Unified.h>

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  
  M5.setTouchButtonHeightByRatio(64); 

  M5.Display.setTextSize(2);
  M5.Display.setTextColor(WHITE);
  M5.Display.fillScreen(BLACK);
  
  // Draw button labels
  M5.Display.setCursor(40, 220);
  M5.Display.print("BtnA");
  M5.Display.setCursor(140, 220);
  M5.Display.print("BtnB");
  M5.Display.setCursor(240, 220);
  M5.Display.print("BtnC");
}

void loop() {
  M5.update();
  
  // Check Button A
  if (M5.BtnA.wasPressed()) {
    M5.Display.fillRect(0, 0, 320, 30, BLACK);
    M5.Display.setCursor(10, 10);
    M5.Display.print("Button A Pressed");
    Serial.println("Button A Pressed");
  }
  
  // Check Button B
  if (M5.BtnB.wasPressed()) {
    M5.Display.fillRect(0, 0, 320, 30, BLACK);
    M5.Display.setCursor(10, 10);
    M5.Display.print("Button B Pressed");
    Serial.println("Button B Pressed");
  }
  
  // Check Button C
  if (M5.BtnC.wasPressed()) {
    M5.Display.fillRect(0, 0, 320, 30, BLACK);
    M5.Display.setCursor(10, 10);
    M5.Display.print("Button C Pressed");
    Serial.println("Button C Pressed");
  }
  
  delay(10);
}