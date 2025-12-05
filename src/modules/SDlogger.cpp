#include "SDLogger.h"
#include <SPI.h>
#include <SD.h>
#include <TimeLib.h>

// ---------- HSPI pins for TTGO T-Beam + SD module ----------
#define HSPI_SCLK 14
#define HSPI_MISO 2
#define HSPI_MOSI 15
#define HSPI_CS   13

// ---------- Module state ----------
static SPIClass *hspi = nullptr;
static bool sd_ok = false;
static const char *LOG_FILE_PATH = "/gpslog.csv";

// Track last log time for interval calculation
static uint32_t lastLogUnix = 0;

// Expanded CSV header (GPS + IMU + BARO)
static const char *CSV_HEADER =
    "unix_time,human_time,interval_s,"
    "lat_i,lon_i,alt_m,sats,"
    "accX_mg,accY_mg,accZ_mg,"
    "gyrX_dps,gyrY_dps,gyrZ_dps,"
    "magX_uT,magY_uT,magZ_uT,temp_C,"
    "press_Pa,baroAlt_m,baroStatus,baroTemp_C";

bool SDLogger::begin()
{
    // Create HSPI bus if not made yet
    if (!hspi) {
        hspi = new SPIClass(HSPI);
        hspi->begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_CS);
        pinMode(HSPI_CS, OUTPUT);
    }

    // Initialize SD card on HSPI (4 MHz)
    if (!SD.begin(HSPI_CS, *hspi, 4000000)) {
        Serial.println("[SDLogger] SD card init failed");
        sd_ok = false;
        return false;
    }

    Serial.println("[SDLogger] SD card initialized yippie");
    sd_ok = true;

    // Create CSV file with expanded header if missing
    if (!SD.exists(LOG_FILE_PATH)) {
        File f = SD.open(LOG_FILE_PATH, FILE_WRITE);
        if (f) {
            f.println(CSV_HEADER);
            f.flush();
            f.close();
        }
    }

    return true;
}

static inline void writeHumanTime(char *buf, size_t bufSz, uint32_t unixTime)
{
    time_t t = (time_t)unixTime;
    snprintf(buf, bufSz, "%04d-%02d-%02d %02d:%02d:%02d",
             year(t), month(t), day(t), hour(t), minute(t), second(t));
}

// ======================
// GPS-only logging
// ======================
void SDLogger::log(const SDGpsFix &fix)
{
    if (!sd_ok) return;
    if (fix.lat_i == 0 && fix.lon_i == 0) return;

    // Calculate interval in seconds
    uint32_t interval = (lastLogUnix == 0) ? 0 : (fix.unixTime - lastLogUnix);
    lastLogUnix = fix.unixTime;

    // Human-readable time
    char timeStr[25];
    writeHumanTime(timeStr, sizeof(timeStr), fix.unixTime);

    File f = SD.open(LOG_FILE_PATH, FILE_APPEND); // Append mode
    if (!f) return;

    // Write GPS portion
    f.print(fix.unixTime);  f.print(',');        // Raw Unix time
    f.print(timeStr);       f.print(',');        // Human-readable time
    f.print(interval);      f.print(',');        // Interval in seconds
    f.print(fix.lat_i);     f.print(',');
    f.print(fix.lon_i);     f.print(',');
    f.print(fix.alt_m);     f.print(',');
    f.print((uint32_t)fix.sats);

    // IMU columns empty to maintain CSV shape (10 IMU fields)
    for (int i = 0; i < 10; ++i) f.print(',');

    // BARO columns empty (4 fields)
    for (int i = 0; i < 4; ++i) f.print(',');

    f.println();
    f.flush();
    f.close();
}

// ======================
// GPS + IMU logging
// ======================
void SDLogger::log(const SDGpsFix &fix, const SDImuSample &imu)
{
    if (!sd_ok) return;
    if (fix.lat_i == 0 && fix.lon_i == 0) return;

    // Calculate interval in seconds
    uint32_t interval = (lastLogUnix == 0) ? 0 : (fix.unixTime - lastLogUnix);
    lastLogUnix = fix.unixTime;

    // Human-readable time
    char timeStr[25];
    writeHumanTime(timeStr, sizeof(timeStr), fix.unixTime);

    File f = SD.open(LOG_FILE_PATH, FILE_APPEND); // Append mode
    if (!f) return;

    // GPS first
    f.print(fix.unixTime);  f.print(',');
    f.print(timeStr);       f.print(',');
    f.print(interval);      f.print(',');
    f.print(fix.lat_i);     f.print(',');
    f.print(fix.lon_i);     f.print(',');
    f.print(fix.alt_m);     f.print(',');
    f.print((uint32_t)fix.sats);  f.print(',');

    // IMU values (scaled) with consistent precision
    f.print(imu.ax_mg, 3);  f.print(',');
    f.print(imu.ay_mg, 3);  f.print(',');
    f.print(imu.az_mg, 3);  f.print(',');
    f.print(imu.gx_dps, 3); f.print(',');
    f.print(imu.gy_dps, 3); f.print(',');
    f.print(imu.gz_dps, 3); f.print(',');
    f.print(imu.mx_uT, 3);  f.print(',');
    f.print(imu.my_uT, 3);  f.print(',');
    f.print(imu.mz_uT, 3);  f.print(',');
    f.print(imu.t_C, 2);    f.print(',');

    // BARO placeholders (press_Pa, baroAlt_m, baroStatus, baroTemp_C)
    for (int i = 0; i < 3; ++i) f.print(',');
    f.println(); // last empty field then newline

    f.flush();
    f.close();
}

// ======================
// GPS + IMU + BARO logging
// ======================
void SDLogger::log(const SDGpsFix &fix, const SDImuSample &imu, const SDBaroSample &baro)
{
    if (!sd_ok) return;
    if (fix.lat_i == 0 && fix.lon_i == 0) return;

    // Calculate interval in seconds
    uint32_t interval = (lastLogUnix == 0) ? 0 : (fix.unixTime - lastLogUnix);
    lastLogUnix = fix.unixTime;

    // Human-readable time
    char timeStr[25];
    writeHumanTime(timeStr, sizeof(timeStr), fix.unixTime);

    File f = SD.open(LOG_FILE_PATH, FILE_APPEND); // Append mode
    if (!f) return;

    // GPS
    f.print(fix.unixTime);  f.print(',');
    f.print(timeStr);       f.print(',');
    f.print(interval);      f.print(',');
    f.print(fix.lat_i);     f.print(',');
    f.print(fix.lon_i);     f.print(',');
    f.print(fix.alt_m);     f.print(',');
    f.print((uint32_t)fix.sats);  f.print(',');

    // IMU
    f.print(imu.ax_mg, 3);  f.print(',');
    f.print(imu.ay_mg, 3);  f.print(',');
    f.print(imu.az_mg, 3);  f.print(',');
    f.print(imu.gx_dps, 3); f.print(',');
    f.print(imu.gy_dps, 3); f.print(',');
    f.print(imu.gz_dps, 3); f.print(',');
    f.print(imu.mx_uT, 3);  f.print(',');
    f.print(imu.my_uT, 3);  f.print(',');
    f.print(imu.mz_uT, 3);  f.print(',');
    f.print(imu.t_C, 2);    f.print(',');

    // BARO
    f.print(baro.pressure_Pa, 1); f.print(',');
    f.print(baro.baroAlt_m, 2);   f.print(',');  // will be NAN for gauge sensors
    f.print(baro.status);         f.print(',');
    f.println(baro.temp_C, 2);    // NAN for SparkFun MicroPressure

    f.flush();
    f.close();
}
