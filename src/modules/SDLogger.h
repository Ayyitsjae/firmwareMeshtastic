#pragma once
#include <Arduino.h>

// --- GPS fix payload (unchanged) ---
struct SDGpsFix
{
    int32_t  lat_i;
    int32_t  lon_i;
    int32_t  alt_m;
    uint8_t  sats;
    uint32_t unixTime;
};

// --- IMU sample payload (unchanged) ---
struct SDImuSample
{
    float ax_mg, ay_mg, az_mg;
    float gx_dps, gy_dps, gz_dps;
    float mx_uT, my_uT, mz_uT;
    float t_C;

    SDImuSample()
        : ax_mg(NAN), ay_mg(NAN), az_mg(NAN),
          gx_dps(NAN), gy_dps(NAN), gz_dps(NAN),
          mx_uT(NAN), my_uT(NAN), mz_uT(NAN),
          t_C(NAN) {}
};

// --- NEW: MicroPressure / barometer sample (optional) ---
struct SDBaroSample
{
    float pressure_Pa;   // gauge pressure in Pascals
    float baroAlt_m;     // NAN for gauge sensors
    int   status;        // status/health code (0 = ok)
    float temp_C;        // always NAN for this device

    SDBaroSample()
        : pressure_Pa(NAN), baroAlt_m(NAN), status(0), temp_C(NAN) {}
};

namespace SDLogger
{
    bool begin();

    // GPS-only row. Leaves IMU + BARO columns blank to keep CSV shape consistent.
    void log(const SDGpsFix &fix);

    // GPS + IMU row. Leaves BARO columns blank.
    void log(const SDGpsFix &fix, const SDImuSample &imu);

    // --- NEW: GPS + IMU + BARO row
    void log(const SDGpsFix &fix, const SDImuSample &imu, const SDBaroSample &baro);
}
