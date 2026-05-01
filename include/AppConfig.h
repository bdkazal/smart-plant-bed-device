#pragma once

#include <Arduino.h>

static const int MAX_WATERING_SCHEDULES = 10;

struct WateringScheduleConfig
{
  int id = 0;
  bool isEnabled = false;
  int dayOfWeek = 0;
  String timeOfDay;
  int durationSeconds = 0;
};

struct DeviceConfig
{
  String deviceName;
  String timezone;
  String wateringMode;
  int soilMoistureThreshold = 0;
  int maxWateringDurationSeconds = 0;
  int cooldownMinutes = 0;
  int localManualDurationSeconds = 0;

  WateringScheduleConfig schedules[MAX_WATERING_SCHEDULES];
  int scheduleCount = 0;

  bool hasLoadedConfig = false;
};

extern DeviceConfig deviceConfig;

void printDeviceConfig();

bool parseConfigResponse(const String &response);
bool parseConfigObjectJson(const String &configJson);

String extractConfigJsonFromResponse(const String &response);
