#pragma once
#include <Arduino.h>
#include "ICM_20948.h"   // your ICM-20948 C++ driver
#include "modules/SDLogger.h"    // for SDImuSample

// Global instance owned by this module (defined in ImuProvider.cpp)
extern ICM_20948_I2C imu;

namespace ImuProvider
{
    // Initialize the IMU (idempotent; safe to call multiple times)
    bool begin(bool ad0High = true, uint8_t ad0Pin = ICM_20948_ARD_UNUSED_PIN);

    // Read one synchronized IMU sample (scaled units ready for CSV)
    SDImuSample readSample();
}
