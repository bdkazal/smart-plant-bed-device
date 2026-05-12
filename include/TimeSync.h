#pragma once

#include <Arduino.h>

void beginTimeSync();
void syncTimeFromNtp(const String &timezoneName);

// Fallback time source when public NTP is unavailable but Laravel is reachable.
// Expected format from Laravel: YYYY-MM-DD HH:MM:SS in the configured local timezone.
bool syncTimeFromLaravelTimestamp(const String &timestamp);

bool isTimeReady();
String getTimeSourceText();

int getCurrentDayOfWeekIso();
String getCurrentDateString();
String getCurrentTimeString();
String getCurrentHourMinuteString();
