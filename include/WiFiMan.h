#pragma once

#include "DeviceStorage.h"

bool isWiFiConnected();

bool connectToWiFiUsingConfig(const StoredDeviceConfig &storedConfig);

void connectToWiFi();