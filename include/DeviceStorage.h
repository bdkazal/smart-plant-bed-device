#pragma once

#include <Arduino.h>

struct StoredDeviceConfig
{
    String wifiSsid;
    String wifiPassword;

    bool hasWifiCredentials = false;
};

void beginDeviceStorage();

StoredDeviceConfig loadStoredDeviceConfig();

bool saveWifiCredentials(const String &ssid, const String &password);

String loadCachedConfigJson();
bool hasCachedConfigJson();
bool saveCachedConfigJsonIfChanged(const String &configJson);

void clearStoredDeviceConfig();
