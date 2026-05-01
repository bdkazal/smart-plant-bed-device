#pragma once

#include <Arduino.h>

void beginTimeSync();
void syncTimeFromNtp();

bool isTimeReady();

int getCurrentDayOfWeekIso();
String getCurrentTimeString();
