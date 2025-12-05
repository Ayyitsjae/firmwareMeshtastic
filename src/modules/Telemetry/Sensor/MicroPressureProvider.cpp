#include "MicroPressureProvider.h"

// Internal state
static SparkFun_MicroPressure mpr;
static bool     ready       = false;
static int      status_last = 0;
static TwoWire* wirePtr     = &Wire;
static uint8_t  i2cAddr     = DEFAULT_ADDRESS;

bool MicroPressureProvider::begin(TwoWire& wire, uint8_t addr)
{
    if (ready) return true;
    wire.begin();
    wirePtr = &wire;
    i2cAddr = addr;

    if (!mpr.begin(addr, wire)) {
        Serial.println("[MicroPressure] begin failed");
        ready = false;
        return false;
    }
    ready = true;
    Serial.println("[MicroPressure] initialized");
    return true;
}

SDBaroSample MicroPressureProvider::readSample(uint32_t budgetMs)
{
    SDBaroSample s;
    const uint32_t start = millis();

    if (!ready) {
        if (!begin(*wirePtr, i2cAddr)) {
            // stays NANs
            return s;
        }
    }

    // Read status first — if BUSY, skip blocking readPressure()
    status_last = mpr.readStatus();
    s.status = status_last;

    // Treat 0xFF as bus error (driver’s pattern)
    if (status_last == 0xFF) {
        ready = false;
        return s; // NANs
    }

    // If sensor indicates busy, avoid blocking wait-loop inside readPressure()
    if (status_last & BUSY_FLAG) {
        // Quick exit: keep NANs, caller will try again later
        return s;
    }

    // Normal path: read pressure in Pascals
    float p_pa = mpr.readPressure(PA);  // returns NAN if integrity/math saturation set
    s.pressure_Pa = p_pa;
    // Gauge sensor: no temp exposed, no barometric altitude
    s.temp_C    = NAN;
    s.baroAlt_m = NAN;

    // Time budget guard (best-effort)
    if ((millis() - start) > budgetMs) {
        // If read was slow, we still return the sample, but mark not-ready so the caller can back off next loop
        ready = false;
    } else {
        ready = true;
    }

    return s;
}

bool MicroPressureProvider::isReady()     { return ready; }
void MicroPressureProvider::markFailed()  { ready = false; }
bool MicroPressureProvider::reinit()      { return begin(*wirePtr, i2cAddr); }
int  MicroPressureProvider::lastStatus()  { return status_last; }
