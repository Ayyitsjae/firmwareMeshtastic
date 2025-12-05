#include "ImuProvider.h"
#include <Wire.h>

ICM_20948_I2C imu;              // <-- global definition resolves the linker error
static bool imuReady = false;

bool ImuProvider::begin(bool ad0High, uint8_t ad0Pin)
{
    if (imuReady) return true;

    Wire.begin(); // I2C bus start (T-Beam default SDA/SCL)
    ICM_20948_Status_e st = imu.begin(Wire, ad0High, ad0Pin);
    if (st != ICM_20948_Stat_Ok) {
        Serial.print("[IMU] begin failed: ");
        Serial.println(imu.statusString(st));
        return false;
    }

    // Optional: configure FS and ODR (aligns with your driver helpers)
    ICM_20948_fss_t fss;
    fss.a = gpm4;      // 4g
    fss.g = dps2000;   // 2000 dps
    imu.setFullScale((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), fss);

    ICM_20948_smplrt_t smplrt;
    smplrt.g = 19;     // ~55 Hz gyro (1.1 kHz / (1+19))
    smplrt.a = 19;     // ~56 Hz accel (1.125 kHz / (1+19))
    imu.setSampleRate((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), smplrt);

    // DLPF preference
    imu.enableDLPF(ICM_20948_Internal_Gyr, true);
    imu.enableDLPF(ICM_20948_Internal_Acc, false);

    imuReady = true;
    Serial.println("[IMU] initialized");
    return true;
}

SDImuSample ImuProvider::readSample()
{
    SDImuSample s;
    if (!imuReady) {
        // Try lazy init once
        if (!begin()) return s; // returns NANs if init fails
    }

    imu.getAGMT();            // updates internal agmt in one shot
    s.ax_mg = imu.accX();     // mg
    s.ay_mg = imu.accY();
    s.az_mg = imu.accZ();
    s.gx_dps = imu.gyrX();    // deg/s
    s.gy_dps = imu.gyrY();
    s.gz_dps = imu.gyrZ();
    s.mx_uT = imu.magX();     // microtesla
    s.my_uT = imu.magY();
    s.mz_uT = imu.magZ();
    s.t_C   = imu.temp();     // °C

    return s;
}
