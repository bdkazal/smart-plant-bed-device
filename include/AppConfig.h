#pragma once

#include <Arduino.h>

struct DeviceConfig
{
  String deviceName;
  String timezone;
  String wateringMode;
  int soilMoistureThreshold = 0;
  int maxWateringDurationSeconds = 0;
  int cooldownMinutes = 0;
  int localManualDurationSeconds = 0;
};

extern DeviceConfig deviceConfig;

void printDeviceConfig();
bool parseConfigResponse(const String &response);