#include "TimeSync.h"

#include <Arduino.h>
#include <time.h>

#include "RtcClock.h"
#include "WiFiMan.h"

static bool timeReady = false;
static String activeTimezoneName = "Asia/Dhaka";
static String timeSourceText = "NONE";

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

void applyTimezone(const String &timezoneName)
{
    activeTimezoneName = timezoneName.length() > 0 ? timezoneName : "Asia/Dhaka";
    const char *posixTimezone = getPosixTimezoneForName(activeTimezoneName);

    setenv("TZ", posixTimezone, 1);
    tzset();

    Serial.print("Active timezone: ");
    Serial.println(activeTimezoneName);
    Serial.print("POSIX timezone: ");
    Serial.println(posixTimezone);
}

void beginTimeSync()
{
    Serial.println();
    Serial.println("Time sync initialized.");

    applyTimezone(activeTimezoneName);
    beginRtcClock();

    if (loadSystemTimeFromRtc())
    {
        timeReady = true;
        timeSourceText = "RTC";
        Serial.println("Time source: RTC backup.");
    }
    else
    {
        timeReady = false;
        timeSourceText = "NONE";
        Serial.println("Time source: none. Waiting for NTP.");
    }
}

void syncTimeFromNtp(const String &timezoneName)
{
    if (!isWiFiConnected())
    {
        Serial.println("Cannot sync time: Wi-Fi is not connected.");

        if (!timeReady && loadSystemTimeFromRtc())
        {
            timeReady = true;
            timeSourceText = "RTC";
            Serial.println("Time source: RTC backup after failed NTP attempt.");
        }

        return;
    }

    applyTimezone(timezoneName);
    const char *posixTimezone = getPosixTimezoneForName(activeTimezoneName);

    Serial.println();
    Serial.println("Syncing time from NTP...");
    Serial.print("Config timezone: ");
    Serial.println(activeTimezoneName);

    configTzTime(
        posixTimezone,
        "pool.ntp.org",
        "time.nist.gov"
    );

    struct tm timeInfo;

    if (!getLocalTime(&timeInfo, 10000))
    {
        Serial.println("Failed to get time from NTP.");

        if (!timeReady && loadSystemTimeFromRtc())
        {
            timeReady = true;
            timeSourceText = "RTC";
            Serial.println("Time source: RTC backup after NTP failure.");
        }

        return;
    }

    timeReady = true;
    timeSourceText = "NTP";

    Serial.print("NTP time synced: ");
    Serial.println(&timeInfo, "%Y-%m-%d %H:%M:%S");

    saveSystemTimeToRtc();
}

bool isTimeReady()
{
    return timeReady;
}

String getTimeSourceText()
{
    return timeSourceText;
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

String getCurrentDateString()
{
    struct tm timeInfo;

    if (!getLocalTime(&timeInfo))
    {
        return "";
    }

    char buffer[11];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeInfo);

    return String(buffer);
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

String getCurrentHourMinuteString()
{
    struct tm timeInfo;

    if (!getLocalTime(&timeInfo))
    {
        return "";
    }

    char buffer[6];
    strftime(buffer, sizeof(buffer), "%H:%M", &timeInfo);

    return String(buffer);
}
