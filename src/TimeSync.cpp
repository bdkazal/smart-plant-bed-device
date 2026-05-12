#include "TimeSync.h"

#include <Arduino.h>
#include <sys/time.h>
#include <time.h>

#include "RtcClock.h"
#include "WiFiMan.h"

static bool timeReady = false;
static String activeTimezoneName = "Asia/Dhaka";
static String timeSourceText = "NONE";

const char *getPosixTimezoneForName(const String &timezoneName)
{
    // POSIX TZ format note:
    // Bangladesh is UTC+6, so POSIX uses BDT-6.
    // The number looks reversed because POSIX stores the offset from local time to UTC.

    if (timezoneName == "Asia/Dhaka")
    {
        return "BDT-6";
    }

    if (timezoneName == "UTC" || timezoneName == "Etc/UTC")
    {
        return "UTC0";
    }

    // V1 fallback.
    // Your current Laravel device timezone is Asia/Dhaka.
    // Add more mappings later if the product supports other countries.
    return "BDT-6";
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

bool parseLaravelLocalTimestamp(const String &timestamp, struct tm &timeInfo)
{
    if (timestamp.length() < 19)
    {
        return false;
    }

    int year = timestamp.substring(0, 4).toInt();
    int month = timestamp.substring(5, 7).toInt();
    int day = timestamp.substring(8, 10).toInt();
    int hour = timestamp.substring(11, 13).toInt();
    int minute = timestamp.substring(14, 16).toInt();
    int second = timestamp.substring(17, 19).toInt();

    if (year < 2025 || year > 2099)
    {
        return false;
    }

    if (month < 1 || month > 12 || day < 1 || day > 31)
    {
        return false;
    }

    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59)
    {
        return false;
    }

    memset(&timeInfo, 0, sizeof(timeInfo));
    timeInfo.tm_year = year - 1900;
    timeInfo.tm_mon = month - 1;
    timeInfo.tm_mday = day;
    timeInfo.tm_hour = hour;
    timeInfo.tm_min = minute;
    timeInfo.tm_sec = second;
    timeInfo.tm_isdst = -1;

    return true;
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
        Serial.println("Time source: none. Waiting for NTP or Laravel server time.");
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
        Serial.println("Failed to get time from NTP. Will use Laravel server time if available.");

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

bool syncTimeFromLaravelTimestamp(const String &timestamp)
{
    if (timestamp.length() == 0)
    {
        return false;
    }

    // NTP is the strongest source. Do not downgrade it with Laravel time.
    if (timeSourceText == "NTP")
    {
        return false;
    }

    struct tm timeInfo;

    if (!parseLaravelLocalTimestamp(timestamp, timeInfo))
    {
        Serial.print("Laravel time sync skipped: invalid timestamp: ");
        Serial.println(timestamp);
        return false;
    }

    time_t epoch = mktime(&timeInfo);

    if (epoch <= 0)
    {
        Serial.println("Laravel time sync skipped: mktime failed.");
        return false;
    }

    struct timeval tv;
    tv.tv_sec = epoch;
    tv.tv_usec = 0;

    if (settimeofday(&tv, nullptr) != 0)
    {
        Serial.println("Laravel time sync failed: settimeofday failed.");
        return false;
    }

    timeReady = true;
    timeSourceText = "LARAVEL";

    Serial.print("System time synced from Laravel: ");
    Serial.println(timestamp);

    saveSystemTimeToRtc();

    return true;
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
