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
        Serial.print(now.year());
        Serial.print("-");
        Serial.print(now.month());
        Serial.print("-");
        Serial.print(now.day());
        Serial.print(" ");
        Serial.print(now.hour());
        Serial.print(":");
        Serial.print(now.minute());
        Serial.print(":");
        Serial.println(now.second());
        return;
    }

    rtcStatusText = "RTC ready";
    Serial.print("DS1307 RTC ready: ");
    Serial.print(now.year());
    Serial.print("-");
    Serial.print(now.month());
    Serial.print("-");
    Serial.print(now.day());
    Serial.print(" ");
    Serial.print(now.hour());
    Serial.print(":");
    Serial.print(now.minute());
    Serial.print(":");
    Serial.println(now.second());
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

    struct timeval tv;
    tv.tv_sec = rtcNow.unixtime();
    tv.tv_usec = 0;

    if (settimeofday(&tv, nullptr) != 0)
    {
        Serial.println("RTC time load failed: settimeofday failed.");
        return false;
    }

    rtcStatusText = "RTC time loaded";

    Serial.print("System time loaded from RTC: ");
    Serial.print(rtcNow.year());
    Serial.print("-");
    Serial.print(rtcNow.month());
    Serial.print("-");
    Serial.print(rtcNow.day());
    Serial.print(" ");
    Serial.print(rtcNow.hour());
    Serial.print(":");
    Serial.print(rtcNow.minute());
    Serial.print(":");
    Serial.println(rtcNow.second());

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

    DateTime systemNow(nowEpoch);

    if (!isReasonableRtcTime(systemNow))
    {
        Serial.println("RTC update skipped: system time is not reasonable.");
        return false;
    }

    rtc.adjust(systemNow);
    rtcTimeValid = true;
    rtcStatusText = "RTC synced from NTP";

    Serial.print("RTC updated from system/NTP time: ");
    Serial.print(systemNow.year());
    Serial.print("-");
    Serial.print(systemNow.month());
    Serial.print("-");
    Serial.print(systemNow.day());
    Serial.print(" ");
    Serial.print(systemNow.hour());
    Serial.print(":");
    Serial.print(systemNow.minute());
    Serial.print(":");
    Serial.println(systemNow.second());

    return true;
}

String getRtcStatusText()
{
    return rtcStatusText;
}
