#include "ImuProvider.h"
#include <Wire.h>

ICM_20948_I2C imu;           // global definition resolves the linker error
static bool imuReady = false;
static bool lastAddrHigh = true; // remember which address worked last

// Allow overriding I2C pins/frequency if needed (T-Beam typically SDA=21, SCL=22)
#ifndef IMU_SDA_PIN
#define IMU_SDA_PIN SDA
#endif
#ifndef IMU_SCL_PIN
#define IMU_SCL_PIN SCL
#endif
#ifndef IMU_I2C_HZ
#define IMU_I2C_HZ 400000
#endif

// Configure FS/ODR/DLPF as in your original code
static void configureImu()
{
    ICM_20948_fss_t fss;
    fss.a = gpm4;     // 4g
    fss.g = dps2000;  // 2000 dps
    imu.setFullScale((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), fss);

    ICM_20948_smplrt_t smplrt;
    smplrt.g = 19; // ~55 Hz gyro (1.1 kHz / (1+19))
    smplrt.a = 19; // ~56 Hz accel (1.125 kHz / (1+19))
    imu.setSampleRate((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), smplrt);

    imu.enableDLPF(ICM_20948_Internal_Gyr, true);
    imu.enableDLPF(ICM_20948_Internal_Acc, false);
}

// Try a single address (ad0High) and set lastAddrHigh on success
static bool tryBeginWithAddr(bool ad0High, uint8_t ad0Pin)
{
    ICM_20948_Status_e st = imu.begin(Wire, ad0High, ad0Pin);
    if (st != ICM_20948_Stat_Ok) {
        Serial.print("[IMU] begin failed: ");
        Serial.println(imu.statusString(st));
        return false;
    }
    lastAddrHigh = ad0High;
    return true;
}

bool ImuProvider::begin(bool ad0High, uint8_t ad0Pin)
{
    if (imuReady) return true;

    // EXPLICIT Wire re-init (pins + freq), then settle
    Wire.begin(IMU_SDA_PIN, IMU_SCL_PIN, IMU_I2C_HZ);
    delay(5);

    // Try requested address, then fall back to opposite
    if (!tryBeginWithAddr(ad0High, ad0Pin)) {
        if (!tryBeginWithAddr(!ad0High, ad0Pin)) {
            imuReady = false;
            return false;
        }
    }

    configureImu();
    imuReady = true;
    Serial.printf("[IMU] initialized (addrHigh=%d)\n", (int)lastAddrHigh);
    return true;
}

SDImuSample ImuProvider::readSample()
{
    SDImuSample s;

    if (!imuReady) {
        if (!begin(lastAddrHigh)) {
            imuReady = false;
            return s; // NANs
        }
    }

    imu.getAGMT(); // update internal snapshot

    // Read values
    s.ax_mg = imu.accX();
    s.ay_mg = imu.accY();
    s.az_mg = imu.accZ();
    s.gx_dps = imu.gyrX();
    s.gy_dps = imu.gyrY();
    s.gz_dps = imu.gyrZ();
    s.mx_uT = imu.magX();
    s.my_uT = imu.magY();
    s.mz_uT = imu.magZ();
    s.t_C   = imu.temp();

    // --- NEW: treat all-zero sample as failure (common post hot-plug) ---
    const bool allGyroZero = (s.gx_dps == 0.0f && s.gy_dps == 0.0f && s.gz_dps == 0.0f);
    const bool allAccelZero = (s.ax_mg == 0.0f && s.ay_mg == 0.0f && s.az_mg == 0.0f);
    if (allGyroZero && allAccelZero) {
        imuReady = false;
        Serial.println("[IMU] zero sample detected - marking failed");
        return SDImuSample{}; // return NANs to trigger recovery
    }

    return s;
}

bool ImuProvider::isReady()    { return imuReady; }
void ImuProvider::markFailed() { imuReady = false; }

bool ImuProvider::softReset()
{
    if (!imuReady) return false;
    ICM_20948_Status_e st = imu.swReset();
    if (st != ICM_20948_Stat_Ok) {
        Serial.print("[IMU] soft reset failed: ");
        Serial.println(imu.statusString(st));
        imuReady = false;
        return false;
    }
    delay(10);
    configureImu();
    Serial.println("[IMU] soft reset complete");
    return true;
}

void ImuProvider::recoverI2CBus(uint8_t sdaPin, uint8_t sclPin)
{
    // 9 pulses on SCL to free the bus, then explicit Wire re-init
    pinMode(sclPin, OUTPUT);
    for (int i = 0; i < 9; ++i) {
        digitalWrite(sclPin, HIGH); delayMicroseconds(5);
        digitalWrite(sclPin, LOW);  delayMicroseconds(5);
    }
    pinMode(sdaPin, INPUT_PULLUP);
    pinMode(sclPin, INPUT_PULLUP);

    Wire.end();
    delay(2);
    Wire.begin(IMU_SDA_PIN, IMU_SCL_PIN, IMU_I2C_HZ);
    delay(5);
    Serial.println("[IMU] I2C bus recovery performed");
}

bool ImuProvider::probeWhoAmI(uint8_t &who)
{
    if (!imuReady) return false;
    who = imu.getWhoAmI();  // typically 0xEA
    return (who != 0xFF);
}