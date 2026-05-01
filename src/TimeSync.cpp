#include "TimeSync.h"

#include <Arduino.h>
#include <time.h>

#include "WiFiMan.h"

static bool timeReady = false;
static String activeTimezoneName = "Asia/Dhaka";

const char *getPosixTimezoneForName(const String &timezoneName)
{
    // POSIX TZ format note:
    // For UTC+6, POSIX uses GMT-6.
    // For UTC-5, POSIX uses EST5.
    // This is confusing, but it is how the C time library expects it.

    if (timezoneName == "Asia/Dhaka")
    {
        return "GMT-6";
    }

    if (timezoneName == "UTC" || timezoneName == "Etc/UTC")
    {
        return "UTC0";
    }

    // V1 fallback.
    // Your current Laravel device timezone is Asia/Dhaka.
    // Add more mappings later if the product supports other countries.
    return "GMT-6";
}

void beginTimeSync()
{
    Serial.println();
    Serial.println("Time sync initialized.");
}

void syncTimeFromNtp(const String &timezoneName)
{
    if (!isWiFiConnected())
    {
        Serial.println("Cannot sync time: Wi-Fi is not connected.");
        timeReady = false;
        return;
    }

    activeTimezoneName = timezoneName.length() > 0 ? timezoneName : "Asia/Dhaka";
    const char *posixTimezone = getPosixTimezoneForName(activeTimezoneName);

    Serial.println();
    Serial.println("Syncing time from NTP...");
    Serial.print("Config timezone: ");
    Serial.println(activeTimezoneName);
    Serial.print("POSIX timezone: ");
    Serial.println(posixTimezone);

    configTzTime(
        posixTimezone,
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
