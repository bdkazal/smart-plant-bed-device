#include "TimeSync.h"

#include <Arduino.h>
#include <time.h>

#include "WiFiMan.h"

// Bangladesh timezone: UTC+6, no daylight saving.
// Later we can map Laravel timezone strings dynamically.
static const long GMT_OFFSET_SECONDS = 6 * 60 * 60;
static const int DAYLIGHT_OFFSET_SECONDS = 0;

static bool timeReady = false;

void beginTimeSync()
{
    Serial.println();
    Serial.println("Time sync initialized.");
}

void syncTimeFromNtp()
{
    if (!isWiFiConnected())
    {
        Serial.println("Cannot sync time: Wi-Fi is not connected.");
        timeReady = false;
        return;
    }

    Serial.println();
    Serial.println("Syncing time from NTP...");

    configTime(
        GMT_OFFSET_SECONDS,
        DAYLIGHT_OFFSET_SECONDS,
        "pool.ntp.org",
        "time.nist.gov"
    );

    struct tm timeInfo;

    if (!getLocalTime(&timeInfo, 10000))
    {
        Serial.println("Failed to get time from NTP.");
        timeReady = false;
        return;
    }

    timeReady = true;

    Serial.print("NTP time synced: ");
    Serial.println(&timeInfo, "%Y-%m-%d %H:%M:%S");
}

bool isTimeReady()
{
    return timeReady;
}

int getCurrentDayOfWeekIso()
{
    struct tm timeInfo;

    if (!getLocalTime(&timeInfo))
    {
        return 0;
    }

    // tm_wday: Sunday=0, Monday=1, ... Saturday=6
    if (timeInfo.tm_wday == 0)
    {
        return 7;
    }

    return timeInfo.tm_wday;
}

String getCurrentTimeString()
{
    struct tm timeInfo;

    if (!getLocalTime(&timeInfo))
    {
        return "";
    }

    char buffer[9];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeInfo);

    return String(buffer);
}
