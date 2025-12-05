#pragma once
#include <Arduino.h>
#include "ICM_20948.h"      // your ICM-20948 C++ driver
#include "modules/SDLogger.h" // for SDImuSample

// Global instance owned by this module (defined in ImuProvider.cpp)
extern ICM_20948_I2C imu;

namespace ImuProvider
{
    // Initialize the IMU (idempotent; safe to call multiple times)
    // Tries the provided address (ad0High) and falls back to the opposite if needed.
    bool begin(bool ad0High = true, uint8_t ad0Pin = ICM_20948_ARD_UNUSED_PIN);

    // Read one synchronized IMU sample (scaled units ready for CSV)
    SDImuSample readSample();

    // Health helpers
    bool isReady();
    void markFailed();

    // ---- NEW recovery helpers ----
    // Attempt a software reset of the IMU and re-apply configuration.
    bool softReset();

    // Try to recover the I2C bus lines; then re-start Wire.
    void recoverI2CBus(uint8_t sdaPin = SDA, uint8_t sclPin = SCL);

    // Probe WHO_AM_I value; returns false if read fails.
    bool probeWhoAmI(uint8_t &who);
}
