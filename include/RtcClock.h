#pragma once

#include <Arduino.h>

void beginRtcClock();

bool isRtcAvailable();
bool isRtcTimeValid();

// Loads RTC time into the ESP32 system clock when the RTC has a valid time.
bool loadSystemTimeFromRtc();

// Saves the current ESP32 system time into the RTC after a successful NTP sync.
bool saveSystemTimeToRtc();

String getRtcStatusText();
