#include <M5Unified.h>
#include <SPI.h>
#include <SD.h>
#include <math.h>

// --- STORAGE CONFIGURATION ---
#define MAX_SAMPLES 500  
struct IMUDataEntry {
    uint32_t timestamp; // Time in milliseconds
    float heading, pitch, roll;
    float accX, accY, accZ;
    float gyroX, gyroY, gyroZ;
};

IMUDataEntry dataLog[MAX_SAMPLES];
int logIndex = 0;
bool bufferFull = false;


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

// --- SD CARD FUNCTION ---
void saveLogToSD() {
    M5.Display.fillScreen(BLACK);
    M5.Display.setCursor(10, 100);
    M5.Display.println("SAVING TO SD...");

    // Create a unique filename or overwrite existing
    File file = SD.open("/imu_log.csv", FILE_WRITE);

    if (file) {
        // Updated CSV Header with Timestamp
        file.println("Timestamp_ms,Heading,Pitch,Roll,AccX,AccY,AccZ,GyroX,GyroY,GyroZ");

        int totalSamples = bufferFull ? MAX_SAMPLES : logIndex;

        for (int i = 0; i < totalSamples; i++) {
            file.printf("%u,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
                        dataLog[i].timestamp, dataLog[i].heading, 
                        dataLog[i].pitch, dataLog[i].roll,
                        dataLog[i].accX, dataLog[i].accY, dataLog[i].accZ,
                        dataLog[i].gyroX, dataLog[i].gyroY, dataLog[i].gyroZ);
        }
        file.close();
        M5.Display.println("SAVE SUCCESS!");
    } else {
        M5.Display.setTextColor(RED);
        M5.Display.println("SD SAVE FAILED!");
        M5.Display.setTextColor(WHITE);
    }
    delay(2000);
    M5.Display.fillScreen(BLACK);
}

// Function updated to accept timestamp
void storeData(uint32_t ts, float h, float p, float r, m5::imu_3d_t a, m5::imu_3d_t g) {
    dataLog[logIndex] = {ts, h, p, r, a.x, a.y, a.z, g.x, g.y, g.z};
    logIndex++;
    if (logIndex >= MAX_SAMPLES) {
        logIndex = 0;
        bufferFull = true;
    }
}

// ... (calibrateMagnetometer function stays the same) ...

void setup() {
    auto cfg = M5.config();
    cfg.internal_imu = true; 
    M5.begin(cfg);
    M5.Display.setRotation(1);
    M5.Display.setTextSize(2);

    // CoreS3 SD Initialization (CS Pin is 4)
    if (!SD.begin(GPIO_NUM_4, SPI, 40000000)) {
        M5.Display.println("SD Init Failed!");
    }

    if (!M5.Imu.begin()) {
        M5.Display.println("IMU Init Failed!");
        while (1) delay(1);
    }
}

void loop() {
    M5.update();

    // Touch logic for Calibration (Center) and Save (Top Right)
    if (M5.Touch.getCount() > 0) {
        auto detail = M5.Touch.getDetail(0);
        if (detail.isPressed()) {
            if (detail.x > 110 && detail.x < 210 && detail.y > 70 && detail.y < 170) {
                calibrateMagnetometer();
            }
            if (detail.x > 240 && detail.y < 80) {
                saveLogToSD();
            }
        }
    }

    M5.Imu.update();
    auto data = M5.Imu.getImuData();

    // Math for Heading/Pitch/Roll
    float rollRad = atan2(data.accel.x, data.accel.z);
    float pitchRad = atan2(-data.accel.y, sqrt(data.accel.x * data.accel.x + data.accel.z * data.accel.z));

    float my = data.mag.x - magOffsetX;
    float mx = data.mag.y - magOffsetY;
    float mz = data.mag.z - magOffsetZ;
    float magX_hor = mx * cos(pitchRad) + mz * sin(pitchRad);
    float magY_hor = mx * sin(rollRad) * sin(pitchRad) + my * cos(rollRad) - mz * sin(rollRad) * cos(pitchRad);
    float heading = atan2(magY_hor, magX_hor) * 180.0 / M_PI;
    if (heading < 0) heading += 360;

    // --- STORE DATA WITH TIMESTAMP ---
    storeData(millis(), heading, pitchRad * 180 / M_PI, rollRad * 180 / M_PI, data.accel, data.gyro);

    // --- DISPLAY ---
    M5.Display.setCursor(10, 10);
    M5.Display.setTextColor(YELLOW, BLACK);
    M5.Display.printf("HEADING: %6.1f deg\n", heading);
    M5.Display.setTextColor(GREEN, BLACK);
    M5.Display.printf("TR: Save SD | Cnt: %d\n", bufferFull ? MAX_SAMPLES : logIndex);

    M5.Display.setTextColor(CYAN, BLACK);
    M5.Display.printf("TILT DATA:\n P: %6.1f R: %6.1f\n", pitchRad * 180 / M_PI, rollRad * 180 / M_PI);

    M5.Display.setTextColor(WHITE, BLACK);
    M5.Display.printf("ACCEL: %.2f %.2f %.2f\n", data.accel.x, data.accel.y, data.accel.z);

    delay(50);
}