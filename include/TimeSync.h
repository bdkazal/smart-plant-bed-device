#pragma once

#include <Arduino.h>

void beginTimeSync();
void syncTimeFromNtp(const String &timezoneName);

bool isTimeReady();

int getCurrentDayOfWeekIso();
String getCurrentDateString();
String getCurrentTimeString();
