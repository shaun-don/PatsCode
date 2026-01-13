#include <M5Unified.h>

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  
  M5.Display.setRotation(1); // Standard landscape
  M5.Display.fillScreen(BLACK);
  
  // Draw a guide bar at the bottom to show the button area
  // We'll place it from Y=180 to Y=240
  M5.Display.drawRect(0, 180, 320, 60, WHITE); 
  M5.Display.drawFastVLine(106, 180, 60, WHITE);
  M5.Display.drawFastVLine(212, 180, 60, WHITE);
  
  M5.Display.setTextSize(2);
  M5.Display.setCursor(10, 10);
  M5.Display.println("CoreS3 Calibrated Test");
}

void loop() {
  M5.update();

  if (M5.Touch.getCount() > 0) {
    auto detail = M5.Touch.getDetail(0);
    M5.Display.fillCircle(detail.x, detail.y-50, 5, WHITE);

    // Only trigger if touch is inside the bottom bar (Y > 180)
    if (detail.y > 180) {
      M5.Display.fillRect(0, 50, 320, 30, BLACK); // Clear status line
      M5.Display.setCursor(10, 50);

      if (detail.x < 106) {
        M5.Display.setTextColor(GREEN);
        M5.Display.print("Pressed: LEFT");
      } 
      else if (detail.x < 212) {
        M5.Display.setTextColor(BLUE);
        M5.Display.print("Pressed: MIDDLE");
      } 
      else {
        M5.Display.setTextColor(RED);
        M5.Display.print("Pressed: RIGHT");
      }
      
      // Visual feedback: Draw a temporary circle where you touched
    }
  }

  // Slowly fade the touch dots to keep the screen clean
  if (millis() % 1000 == 0) {
     // Optional: Clear the top area occasionally
     // M5.Display.fillRect(0, 80, 320, 100, BLACK);
  }
}