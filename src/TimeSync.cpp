#include "TimeSync.h"

#include <Arduino.h>
#include <sys/time.h>
#include <time.h>

#include "RtcClock.h"
#include "WiFiMan.h"

static bool timeReady = false;
static String activeTimezoneName = "Asia/Dhaka";
static String timeSourceText = "NONE";
static String activePosixTimezone = "UTC-6";

String buildPosixTimezoneFromOffsetMinutes(int offsetMinutes)
{
    // Laravel sends normal timezone offset minutes:
    //   Asia/Dhaka UTC+6  =>  360
    //   UTC               =>  0
    //   America/New_York EST example => -300
    //
    // POSIX TZ signs are reversed:
    //   UTC+6  => UTC-6
    //   UTC-5  => UTC5
    //
    // This fixed-offset format follows the server-selected timezone offset.
    // It does not encode future DST rule changes, but Laravel will refresh the
    // offset in config when the device is online.

    int totalMinutes = abs(offsetMinutes);
    int hours = totalMinutes / 60;
    int minutes = totalMinutes % 60;

    String posix = "UTC";

    if (offsetMinutes > 0)
    {
        posix += "-";
    }
    else if (offsetMinutes < 0)
    {
        posix += "+";
    }
    else
    {
        posix += "0";
        return posix;
    }

    posix += String(hours);

    if (minutes > 0)
    {
        posix += ":";
        if (minutes < 10)
        {
            posix += "0";
        }
        posix += String(minutes);
    }

    return posix;
}

void applyTimezone(const String &timezoneName, int timezoneOffsetMinutes)
{
    activeTimezoneName = timezoneName.length() > 0 ? timezoneName : "Asia/Dhaka";
    activePosixTimezone = buildPosixTimezoneFromOffsetMinutes(timezoneOffsetMinutes);

    setenv("TZ", activePosixTimezone.c_str(), 1);
    tzset();

    Serial.print("Active timezone: ");
    Serial.println(activeTimezoneName);
    Serial.print("Timezone offset minutes: ");
    Serial.println(timezoneOffsetMinutes);
    Serial.print("POSIX timezone: ");
    Serial.println(activePosixTimezone);
}

bool parseBasicTimestamp(const String &timestamp, struct tm &timeInfo)
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

bool parseLaravelLocalTimestamp(const String &timestamp, struct tm &timeInfo)
{
    return parseBasicTimestamp(timestamp, timeInfo);
}

bool parseLaravelUtcTimestamp(const String &timestamp, struct tm &timeInfo)
{
    return parseBasicTimestamp(timestamp, timeInfo);
}

long daysFromCivil(int year, unsigned month, unsigned day)
{
    // Portable civil-date to days-since-1970 conversion.
    // This avoids timegm(), which is not available in the current ESP32 toolchain.
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;

    return era * 146097L + static_cast<long>(doe) - 719468L;
}

time_t utcTmToEpoch(const struct tm &utcInfo)
{
    int year = utcInfo.tm_year + 1900;
    unsigned month = utcInfo.tm_mon + 1;
    unsigned day = utcInfo.tm_mday;

    long days = daysFromCivil(year, month, day);

    return static_cast<time_t>(
        days * 86400L +
        utcInfo.tm_hour * 3600L +
        utcInfo.tm_min * 60L +
        utcInfo.tm_sec
    );
}

void setSystemTimeFromEpoch(time_t epoch, const String &sourceLabel, const String &originalTimestamp)
{
    struct timeval tv;
    tv.tv_sec = epoch;
    tv.tv_usec = 0;

    if (settimeofday(&tv, nullptr) != 0)
    {
        Serial.print(sourceLabel);
        Serial.println(" time sync failed: settimeofday failed.");
        return;
    }

    timeReady = true;
    timeSourceText = sourceLabel;

    Serial.print("System time synced from ");
    Serial.print(sourceLabel);
    Serial.print(": ");
    Serial.println(originalTimestamp);

    saveSystemTimeToRtc();
}

void beginTimeSync()
{
    Serial.println();
    Serial.println("Time sync initialized.");

    applyTimezone(activeTimezoneName, 360);
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

void syncTimeFromNtp(const String &timezoneName, int timezoneOffsetMinutes)
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

    applyTimezone(timezoneName, timezoneOffsetMinutes);

    Serial.println();
    Serial.println("Syncing time from NTP...");
    Serial.print("Config timezone: ");
    Serial.println(activeTimezoneName);

    configTzTime(
        activePosixTimezone.c_str(),
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

bool syncTimeFromLaravelUtcTimestamp(const String &timestamp)
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

    struct tm utcInfo;

    if (!parseLaravelUtcTimestamp(timestamp, utcInfo))
    {
        Serial.print("Laravel UTC time sync skipped: invalid timestamp: ");
        Serial.println(timestamp);
        return false;
    }

    time_t utcEpoch = utcTmToEpoch(utcInfo);

    if (utcEpoch <= 0)
    {
        Serial.println("Laravel UTC time sync skipped: UTC epoch conversion failed.");
        return false;
    }

    setSystemTimeFromEpoch(utcEpoch, "LARAVEL_UTC", timestamp);

    return true;
}

bool syncTimeFromLaravelTimestamp(const String &timestamp)
{
    if (timestamp.length() == 0)
    {
        return false;
    }

    // NTP and Laravel UTC are stronger sources.
    if (timeSourceText == "NTP" || timeSourceText == "LARAVEL_UTC")
    {
        return false;
    }

    struct tm timeInfo;

    if (!parseLaravelLocalTimestamp(timestamp, timeInfo))
    {
        Serial.print("Laravel local time sync skipped: invalid timestamp: ");
        Serial.println(timestamp);
        return false;
    }

    time_t epoch = mktime(&timeInfo);

    if (epoch <= 0)
    {
        Serial.println("Laravel local time sync skipped: mktime failed.");
        return false;
    }

    setSystemTimeFromEpoch(epoch, "LARAVEL_LOCAL", timestamp);

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
