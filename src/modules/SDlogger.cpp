#include "SDLogger.h"
#include <SPI.h>
#include <SD.h>

// HSPI pins for TTGO T-Beam + SD module
#define HSPI_SCLK 14
#define HSPI_MISO 2
#define HSPI_MOSI 15
#define HSPI_CS   13

static SPIClass *hspi = nullptr;
static bool sd_ok = false;
static const char *LOG_FILE_PATH = "/gpslog.csv";

bool SDLogger::begin()
{
    // Create HSPI bus if not made yet
    if (!hspi) {
        hspi = new SPIClass(HSPI);
        hspi->begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_CS);
        pinMode(HSPI_CS, OUTPUT);
    }

    // Initialize SD card on HSPI
    if (SD.begin(HSPI_CS, *hspi, 4000000)) {   // 4 MHz
        Serial.println("[SDLogger] SD card init failed");
        sd_ok = false;
        return false;
    }

    Serial.println("[SDLogger] SD card initialized");
    sd_ok = true;

    // Create CSV file with header if missing
    if (!SD.exists(LOG_FILE_PATH)) {
        File f = SD.open(LOG_FILE_PATH, FILE_WRITE);
        if (f) {
            f.println("t,lat_i,lon_i,alt_m,sats");
            f.close();
        }
    }

    return true;
}

void SDLogger::log(const SDGpsFix &fix)
{
    if (!sd_ok) return;
    if (fix.lat_i == 0 && fix.lon_i == 0) return;

    File f = SD.open(LOG_FILE_PATH, FILE_WRITE);
    if (!f) return;

    f.print(fix.unixTime);  f.print(',');
    f.print(fix.lat_i);     f.print(',');
    f.print(fix.lon_i);     f.print(',');
    f.print(fix.alt_m);     f.print(',');
    f.println(fix.sats);

    f.close();
}
