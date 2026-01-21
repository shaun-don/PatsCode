#include <M5Unified.h>
#include <SPI.h>
#include <SD.h>
#include <math.h>

// --- STORAGE CONFIGURATION ---
#define MAX_SAMPLES 1200  
struct IMUDataEntry {
    uint32_t timestamp; // Time in milliseconds
    float heading, pitch, roll;
    float accX, accY, accZ;
    float gyroX, gyroY, gyroZ;
};

IMUDataEntry dataLog[MAX_SAMPLES];
int logIndex = 0;
bool bufferFull = false;


const int btnX = 110, btnY = 180, btnW = 100, btnH = 40; // Positioned near bottom-middle
void drawLoggingButton(bool bStop = true) {

    if (bStop) {
        M5.Display.fillRoundRect(btnX, btnY, btnW, btnH, 8, RED);
        M5.Display.setTextColor(WHITE);
        M5.Display.drawCenterString("STOP", btnX + (btnW / 2), btnY + 10);
    } else {
        M5.Display.fillRoundRect(btnX, btnY, btnW, btnH, 8, GREEN);
        M5.Display.setTextColor(BLACK);
        M5.Display.drawCenterString("START", btnX + (btnW / 2), btnY + 10);
    }
}

void drawCalibrateButton() {
        M5.Display.fillRoundRect(btnX, btnY-170, btnW+20, btnH, 8, GREEN);
        M5.Display.setTextColor(BLACK);
        M5.Display.drawCenterString("Calibrate", btnX + ((btnW+20) / 2), btnY -160);
}

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
            int index = i;
            if (bufferFull)
            {
                // samples wrapped round array so start at current position and wrap
                index = (logIndex + i) % MAX_SAMPLES;
            }
            file.printf("%u,%u,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n", index,
                        dataLog[index].timestamp, dataLog[index].heading, 
                        dataLog[index].pitch, dataLog[index].roll,
                        dataLog[index].accX, dataLog[index].accY, dataLog[index].accZ,
                        dataLog[index].gyroX, dataLog[index].gyroY, dataLog[index].gyroZ);
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
static unsigned int lastMillis;

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

bool bIsLogging = false;

void loop() {
    static bool bRedraw = true;

    M5.update();

    // Touch logic for Calibration (Center) and Save (Top Right)
    if (M5.Touch.getCount() > 0) {
        auto detail = M5.Touch.getDetail(0);
        if (detail.isPressed()) {
            if (detail.x > btnX && (detail.y > (btnY-170)) && (detail.y < (btnY-140))) {
                calibrateMagnetometer();
                bRedraw = true;
            }
            if (detail.x > btnX && detail.y > btnY) {
                if(!bIsLogging)
                {
                    logIndex = 0;
                    bufferFull = false;
                    bIsLogging = true;
                    //lastMillis = millis();  // only an issue on first iteration which will never happen
                    bRedraw = true;
                }
                else if (logIndex>10)
                {
                    saveLogToSD();
                    bIsLogging = false;
                    bRedraw = true;
                }
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
    unsigned long int m = millis();
    unsigned long int timestamp = m - lastMillis;
    lastMillis = m;
    if (timestamp<100) 
    {
        unsigned long int d =  100-timestamp;
        delay(d);
        lastMillis += d;
        //timestamp = 100;
    }
    storeData(timestamp, heading, pitchRad * 180 / M_PI, rollRad * 180 / M_PI, data.accel, data.gyro);

    // --- DISPLAY ---
    if (bRedraw)
    {
        bRedraw = false;
        drawLoggingButton(bIsLogging);
        drawCalibrateButton();
    }
    M5.Display.setCursor(10, 80);
    M5.Display.setTextColor(YELLOW, BLACK);
    M5.Display.printf("HEADING: %6.1f deg\n", heading);
    M5.Display.setTextColor(GREEN, BLACK);
    M5.Display.printf("TR: Save SD | Cnt: %d\n", bufferFull ? MAX_SAMPLES : logIndex);

    M5.Display.setTextColor(CYAN, BLACK);
    M5.Display.printf("TILT DATA:\n P: %6.1f R: %6.1f\n", pitchRad * 180 / M_PI, rollRad * 180 / M_PI);

    M5.Display.setTextColor(WHITE, BLACK);
    M5.Display.printf("ACCEL: %.2f %.2f %.2f\n", data.accel.x, data.accel.y, data.accel.z);

    //delay(50);
}