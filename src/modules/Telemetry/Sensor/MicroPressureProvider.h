#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "modules/SDLogger.h"
#include "SparkFun_MicroPressure.h"  // your driver (DEFAULT_ADDRESS, BUSY_FLAG, etc.)

namespace MicroPressureProvider
{
    // Initialize sensor (gauge, default addr 0x18). Idempotent.
    bool begin(TwoWire& wire = Wire, uint8_t addr = DEFAULT_ADDRESS);

    // Non-blocking read: if device is busy/unavailable, returns NANs quickly
    SDBaroSample readSample(uint32_t budgetMs = 5);

    // Health helpers
    bool isReady();
    void markFailed();        // external signal (optional)
    bool reinit();            // quick reinit attempt (no backoff)
    int  lastStatus();        // last status byte read
}
