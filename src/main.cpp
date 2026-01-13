#include <M5Unified.h>
#include <math.h>

// Calibration offsets
float magOffsetX = 0, magOffsetY = 0, magOffsetZ = 0;

void calibrateMagnetometer() {
    M5.Display.fillScreen(BLUE);
    M5.Display.setTextColor(WHITE);
    M5.Display.setCursor(10, 80);
    M5.Display.println("CALIBRATING...");
    M5.Display.println("Rotate CoreS3 in a");
    M5.Display.println("Figure-8 for 10s");

    float minX = 1000, maxX = -1000;
    float minY = 1000, maxY = -1000;
    float minZ = 1000, maxZ = -1000;

    uint32_t startTime = millis();
    while (millis() - startTime < 10000) { // 10 second calibration
        M5.Imu.update();
        auto data = M5.Imu.getImuData();

        if (data.mag.x < minX) minX = data.mag.x;
        if (data.mag.x > maxX) maxX = data.mag.x;
        if (data.mag.y < minY) minY = data.mag.y;
        if (data.mag.y > maxY) maxY = data.mag.y;
        if (data.mag.z < minZ) minZ = data.mag.z;
        if (data.mag.z > maxZ) maxZ = data.mag.z;
        
        delay(20);
    }

    // Calculate hard-iron offsets (average of min/max)
    magOffsetX = (maxX + minX) / 2;
    magOffsetY = (maxY + minY) / 2;
    magOffsetZ = (maxZ + minZ) / 2;

    M5.Display.fillScreen(GREEN);
    M5.Display.setCursor(10, 100);
    M5.Display.println("CALIBRATION DONE!");
    delay(1500);
    M5.Display.fillScreen(BLACK);
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(1);
    M5.Display.setTextSize(2);

    if (!M5.Imu.begin()) {
        M5.Display.println("IMU Init Failed!");
        while (1) delay(1);
    }

    calibrateMagnetometer();
}

void loop() {
    M5.Imu.update();
    auto data = M5.Imu.getImuData();

    // Apply Calibration Offsets
    float mx = data.mag.x - magOffsetX;
    float my = data.mag.y - magOffsetY;
    float mz = data.mag.z - magOffsetZ;

    // 1. Calculate Pitch and Roll from Accel
    float pitch = atan2(-data.accel.x, sqrt(data.accel.y * data.accel.y + data.accel.z * data.accel.z));
    float roll  = atan2(data.accel.y, data.accel.z);

    // 2. Tilt Compensation using calibrated values
    float magX_hor = mx * cos(pitch) + mz * sin(pitch);
    float magY_hor = mx * sin(roll) * sin(pitch) + my * cos(roll) - mz * sin(roll) * cos(pitch);

    // 3. Final Heading
    float heading = atan2(magY_hor, magX_hor) * 180.0 / M_PI;
    if (heading < 0) heading += 360;

    // Display
    M5.Display.setCursor(0, 40);
    M5.Display.setTextColor(WHITE, BLACK);
    M5.Display.printf("Heading: %6.1f\n", heading);
    M5.Display.printf("Pitch:   %6.1f\n", pitch * 180 / M_PI);
    M5.Display.printf("Roll:    %6.1f\n", roll * 180 / M_PI);
    
    delay(100);
}
