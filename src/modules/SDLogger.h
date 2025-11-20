#pragma once
#include <Arduino.h>

struct SDGpsFix
{
    int32_t lat_i;
    int32_t lon_i;
    int32_t alt_m;
    uint8_t sats;
    uint32_t unixTime;
};

namespace SDLogger
{
    bool begin();
    void log(const SDGpsFix &fix);
}
