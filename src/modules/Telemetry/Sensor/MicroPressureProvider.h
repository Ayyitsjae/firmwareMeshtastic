#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "modules/SDLogger.h"
#include "SparkFun_MicroPressure.h"  // your driver

namespace MicroPressureProvider
{
    // Initialize the sensor. Returns true if ready.
    // addr: default 0x18 for this device
    bool begin(TwoWire& wire = Wire, uint8_t addr = DEFAULT_ADDRESS);

    // Read one sample (pressure in Pa). For this gauge sensor, baroAlt_m = NAN, temp_C = NAN.
    SDBaroSample readSample();

    // Optional: expose last status if you want external checks
    int lastStatus();
}
