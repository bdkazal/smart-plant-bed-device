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
        Serial.print("DS1307 RTC time invalid: ");
        printDateTime(now);
        return;
    }

    rtcStatusText = "RTC ready";
    Serial.print("DS1307 RTC ready: ");
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

    // DS1307 stores local wall-clock time, not UTC epoch time.
    // Convert that local wall-clock time through mktime(), which respects the active TZ.
    struct tm localTime;
    memset(&localTime, 0, sizeof(localTime));
    localTime.tm_year = rtcNow.year() - 1900;
    localTime.tm_mon = rtcNow.month() - 1;
    localTime.tm_mday = rtcNow.day();
    localTime.tm_hour = rtcNow.hour();
    localTime.tm_min = rtcNow.minute();
    localTime.tm_sec = rtcNow.second();
    localTime.tm_isdst = -1;

    time_t localEpoch = mktime(&localTime);

    if (localEpoch <= 0)
    {
        Serial.println("RTC time load failed: mktime failed.");
        return false;
    }

    struct timeval tv;
    tv.tv_sec = localEpoch;
    tv.tv_usec = 0;

    if (settimeofday(&tv, nullptr) != 0)
    {
        Serial.println("RTC time load failed: settimeofday failed.");
        return false;
    }

    rtcStatusText = "RTC time loaded";

    Serial.print("System time loaded from RTC local time: ");
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

    struct tm localTime;

    if (!localtime_r(&nowEpoch, &localTime))
    {
        Serial.println("RTC update skipped: localtime conversion failed.");
        return false;
    }

    DateTime systemLocalNow(
        localTime.tm_year + 1900,
        localTime.tm_mon + 1,
        localTime.tm_mday,
        localTime.tm_hour,
        localTime.tm_min,
        localTime.tm_sec
    );

    if (!isReasonableRtcTime(systemLocalNow))
    {
        Serial.println("RTC update skipped: local system time is not reasonable.");
        return false;
    }

    rtc.adjust(systemLocalNow);
    rtcTimeValid = true;
    rtcStatusText = "RTC synced from system time";

    Serial.print("RTC updated from local system time: ");
    printDateTime(systemLocalNow);

    return true;
}

String getRtcStatusText()
{
    return rtcStatusText;
}
