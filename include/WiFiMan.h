#pragma once

#include "DeviceStorage.h"

bool isWiFiConnected();

bool connectToWiFiUsingConfig(const StoredDeviceConfig &storedConfig);

void connectToWiFi();

// Non-blocking reconnect helper for the main loop.
// It starts a Wi-Fi reconnect attempt at a safe interval and returns immediately
// so physical buttons and OLED controls remain responsive while offline.
void updateWiFiReconnect();