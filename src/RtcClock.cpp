#include "RtcClock.h"

#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>
#include <sys/time.h>
#include <time.h>

#include "PinConfig.h"

RTC_DS1307 rtc;

static bool rtcAvailable = false;
static bool rtcTimeValid = false;
static String rtcStatusText = "RTC not initialized";

bool isReasonableRtcTime(const DateTime &dateTime)
{
    int year = dateTime.year();

    return year >= 2025 && year <= 2099;
}

void printDateTime(const DateTime &dateTime)
{
    Serial.print(dateTime.year());
    Serial.print("-");
    Serial.print(dateTime.month());
    Serial.print("-");
    Serial.print(dateTime.day());
    Serial.print(" ");
    Serial.print(dateTime.hour());
    Serial.print(":");
    Serial.print(dateTime.minute());
    Serial.print(":");
    Serial.println(dateTime.second());
}

void beginRtcClock()
{
    Serial.println();
    Serial.println("Initializing DS1307 RTC...");

    // OLED and DS1307 share the same I2C bus.
    Wire.begin(OLED_I2C_SDA_PIN, OLED_I2C_SCL_PIN);

    rtcAvailable = rtc.begin();

    if (!rtcAvailable)
    {
        rtcTimeValid = false;
        rtcStatusText = "RTC not found";
        Serial.println("DS1307 RTC not found on I2C bus.");
        return;
    }

    if (!rtc.isrunning())
    {
        rtcTimeValid = false;
        rtcStatusText = "RTC stopped";
        Serial.println("DS1307 RTC found but oscillator is stopped. RTC time is not trusted yet.");
        return;
    }

    DateTime now = rtc.now();
    rtcTimeValid = isReasonableRtcTime(now);

    if (!rtcTimeValid)
    {
        rtcStatusText = "RTC invalid";
        Serial.print("DS1307 RTC UTC time invalid: ");
        printDateTime(now);
        return;
    }

    rtcStatusText = "RTC ready";
    Serial.print("DS1307 RTC ready UTC: ");
    printDateTime(now);
}

bool isRtcAvailable()
{
    return rtcAvailable;
}

bool isRtcTimeValid()
{
    return rtcAvailable && rtcTimeValid;
}

bool loadSystemTimeFromRtc()
{
    if (!isRtcTimeValid())
    {
        Serial.println("RTC time load skipped: RTC is unavailable or invalid.");
        return false;
    }

    DateTime rtcNow = rtc.now();

    if (!isReasonableRtcTime(rtcNow))
    {
        rtcTimeValid = false;
        rtcStatusText = "RTC invalid";
        Serial.println("RTC time load failed: RTC time became invalid.");
        return false;
    }

    // DS1307 stores UTC wall-clock time for stable offline backup.
    // RTClib DateTime::unixtime() treats the stored value as UTC epoch.
    struct timeval tv;
    tv.tv_sec = rtcNow.unixtime();
    tv.tv_usec = 0;

    if (settimeofday(&tv, nullptr) != 0)
    {
        Serial.println("RTC UTC time load failed: settimeofday failed.");
        return false;
    }

    rtcStatusText = "RTC UTC time loaded";

    Serial.print("System time loaded from RTC UTC: ");
    printDateTime(rtcNow);

    return true;
}

bool saveSystemTimeToRtc()
{
    if (!rtcAvailable)
    {
        Serial.println("RTC update skipped: RTC is not available.");
        return false;
    }

    time_t nowEpoch;
    time(&nowEpoch);

    if (nowEpoch <= 0)
    {
        Serial.println("RTC update skipped: system time is not ready.");
        return false;
    }

    struct tm utcTime;

    if (!gmtime_r(&nowEpoch, &utcTime))
    {
        Serial.println("RTC update skipped: UTC conversion failed.");
        return false;
    }

    DateTime systemUtcNow(
        utcTime.tm_year + 1900,
        utcTime.tm_mon + 1,
        utcTime.tm_mday,
        utcTime.tm_hour,
        utcTime.tm_min,
        utcTime.tm_sec
    );

    if (!isReasonableRtcTime(systemUtcNow))
    {
        Serial.println("RTC update skipped: UTC system time is not reasonable.");
        return false;
    }

    rtc.adjust(systemUtcNow);
    rtcTimeValid = true;
    rtcStatusText = "RTC synced from UTC system time";

    Serial.print("RTC updated from UTC system time: ");
    printDateTime(systemUtcNow);

    return true;
}

String getRtcStatusText()
{
    return rtcStatusText;
}
