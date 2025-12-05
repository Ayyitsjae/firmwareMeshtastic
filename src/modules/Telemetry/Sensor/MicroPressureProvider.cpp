#include "MicroPressureProvider.h"

static SparkFun_MicroPressure mpr;
static bool baroReady = false;
static int  last_status = 0;
static TwoWire* wirePtr = &Wire;
static uint8_t i2cAddr = DEFAULT_ADDRESS;

bool MicroPressureProvider::begin(TwoWire& wire, uint8_t addr)
{
    if (baroReady) return true;
    wire.begin();
    wirePtr = &wire;
    i2cAddr = addr;

    if (!mpr.begin(addr, wire)) {
        Serial.println("[MicroPressure] begin failed");
        baroReady = false;
        return false;
    }

    baroReady = true;
    Serial.println("[MicroPressure] initialized");
    return true;
}

SDBaroSample MicroPressureProvider::readSample()
{
    SDBaroSample s;

    if (!baroReady) {
        if (!begin(*wirePtr, i2cAddr)) return s; // will return NANs
    }

    // Read status first (optional, but lets us populate s.status when busy/error)
    last_status = mpr.readStatus();
    // If busy (0x20), we'll still attempt a proper readPressure which handles waiting internally
    // Your driver readPressure() does its own wait loop on EOC/status.

    float p_pa = mpr.readPressure(PA);
    // If integrity or math saturation flagged during the inner request, readPressure() returns NAN.

    s.pressure_Pa = p_pa;
    s.status = last_status;

    // This device doesn't expose temperature, leave NAN.
    s.temp_C = NAN;

    // Gauge sensor: no barometric altitude
    s.baroAlt_m = NAN;

    return s;
}

int MicroPressureProvider::lastStatus()
{
    return last_status;
}
