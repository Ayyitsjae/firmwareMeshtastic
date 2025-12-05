#include "ImuProvider.h"
#include <Wire.h>

ICM_20948_I2C imu;           // global definition resolves the linker error
static bool imuReady = false;
static bool lastAddrHigh = true; // remember which address worked last

// Configure FS/ODR/DLPF as in your original code
static void configureImu()
{
    // Full-scale ranges
    ICM_20948_fss_t fss;
    fss.a = gpm4;     // 4g
    fss.g = dps2000;  // 2000 dps
    imu.setFullScale((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), fss);

    // Sample rates
    ICM_20948_smplrt_t smplrt;
    smplrt.g = 19; // ~55 Hz gyro (1.1 kHz / (1+19))
    smplrt.a = 19; // ~56 Hz accel (1.125 kHz / (1+19))
    imu.setSampleRate((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), smplrt);

    // DLPF preference
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

    Wire.begin(); // I2C bus start (T-Beam default SDA/SCL)

    // Try the requested address first, then fall back to the opposite
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
        // Lazy re-init using last working address preference
        if (!begin(lastAddrHigh)) {
            // Leave NANs; mark failure so caller can back off
            imuReady = false;
            return s;
        }
    }

    // --- FIX: do not assign getAGMT() to a Status_e ---
    imu.getAGMT();  // updates internal agmt snapshot

    // Optional: check driver status (if your driver exposes it)
    // if (imu.status != ICM_20948_Stat_Ok) {
    //     Serial.print("[IMU] getAGMT failed: ");
    //     Serial.println(imu.statusString(imu.status));
    //     imuReady = false;
    //     return s; // NANs
    // }

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
    return s;
}


bool ImuProvider::isReady()    { return imuReady; }
void ImuProvider::markFailed() { imuReady = false; }

// ---- NEW: recovery helpers ----

bool ImuProvider::softReset()
{
    if (!imuReady) return false;

    // If your driver exposes swReset(), use it
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
    // Generate 9 clock pulses on SCL to flush a stuck device, then re-init Wire
    pinMode(sclPin, OUTPUT);
    for (int i = 0; i < 9; ++i) {
        digitalWrite(sclPin, HIGH);
        delayMicroseconds(5);
        digitalWrite(sclPin, LOW);
        delayMicroseconds(5);
    }
    // Release lines
    pinMode(sdaPin, INPUT_PULLUP);
    pinMode(sclPin, INPUT_PULLUP);

    Wire.end();
    delay(2);
    Wire.begin();
    Serial.println("[IMU] I2C bus recovery performed");
}

bool ImuProvider::probeWhoAmI(uint8_t &who)
{
    if (!imuReady) return false;
    who = imu.getWhoAmI();  // WHO_AM_I typically 0xEA for ICM-20948
    return (who != 0xFF);   // 0xFF suggests read error
}
