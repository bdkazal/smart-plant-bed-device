#pragma once

#include <Arduino.h>

void beginTimeSync();
void syncTimeFromNtp(const String &timezoneName);

bool isTimeReady();
String getTimeSourceText();

int getCurrentDayOfWeekIso();
String getCurrentDateString();
String getCurrentTimeString();
String getCurrentHourMinuteString();
