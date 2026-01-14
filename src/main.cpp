#include <M5Unified.h>
#include <math.h>

float magOffsetX = 0, magOffsetY = 0, magOffsetZ = 0;


void calibrateMagnetometer() {
    M5.Display.fillScreen(BLUE);
    M5.Display.setCursor(10, 80);
    M5.Display.println("CALIBRATING...");
    M5.Display.println("Rotate CoreS3 in a Figure-8");

    float minX = 1000, maxX = -1000;
    float minY = 1000, maxY = -1000;
    float minZ = 1000, maxZ = -1000;

    uint32_t startTime = millis();
    while (millis() - startTime < 10000) { 
        M5.Imu.update();
        auto data = M5.Imu.getImuData();
        if (data.mag.x < minX) minX = data.mag.x;
        if (data.mag.x > maxX) maxX = data.mag.x;
        if (data.mag.y < minY) minY = data.mag.y;
        if (data.mag.y > maxY) maxY = data.mag.y;
        if (data.mag.z < minZ) minZ = data.mag.z;
        if (data.mag.z > maxZ) maxZ = data.mag.z;
        delay(10);
    }
    magOffsetX = (maxX + minX) / 2;
    magOffsetY = (maxY + minY) / 2;
    magOffsetZ = (maxZ + minZ) / 2;
    M5.Display.fillScreen(BLACK);
}

void setup() {
    auto cfg = M5.config();
    cfg.internal_imu = true; // Ensure IMU is powered on
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

    // 1. SWAPPED PITCH/ROLL FOR CORES3 ORIENTATION
    // Roll (Tilting side-to-side)
    float rollRad = atan2(data.accel.x, data.accel.z);
    // Pitch (Tilting front-to-back)
    float pitchRad = atan2(-data.accel.y, sqrt(data.accel.x * data.accel.x + data.accel.z * data.accel.z));

    // 2. Tilt-Compensated Heading - Swapped x and y for correct heading
    float my = data.mag.x - magOffsetX;
    float mx = data.mag.y - magOffsetY;
    float mz = data.mag.z - magOffsetZ;
    float magX_hor = mx * cos(pitchRad) + mz * sin(pitchRad);
    float magY_hor = mx * sin(rollRad) * sin(pitchRad) + my * cos(rollRad) - mz * sin(rollRad) * cos(pitchRad);
    float heading = atan2(magY_hor, magX_hor) * 180.0 / M_PI;
    if (heading < 0) heading += 360;

    // --- DISPLAY OUTPUT ---
    M5.Display.setCursor(10, 10);
    
    M5.Display.setTextColor(YELLOW, BLACK);
    M5.Display.printf("HEADING: %6.1f deg\n\n", heading);

    M5.Display.setTextColor(CYAN, BLACK);
    M5.Display.printf("TILT DATA:\n");
    M5.Display.printf(" Pitch: %6.1f\n", pitchRad * 180 / M_PI);
    M5.Display.printf(" Roll:  %6.1f\n\n", rollRad * 180 / M_PI);

    // FIXED ACCELERATION DISPLAY
    M5.Display.setTextColor(WHITE, BLACK);
    M5.Display.printf("ACCEL (G):\n");
    M5.Display.printf(" X:%5.2f Y:%5.2f Z:%5.2f\n", data.accel.x, data.accel.y, data.accel.z);

    // GYROSCOPE (Degrees Per Second)
    // Values represent how fast the device is spinning around each axis
    M5.Display.setTextColor(ORANGE, BLACK);
    M5.Display.printf("GYRO\n X:%5.1f Y:%5.1f Z:%5.1f\n", data.gyro.x, data.gyro.y, data.gyro.z);

    delay(50);
}
