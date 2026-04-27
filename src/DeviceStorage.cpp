#include "DeviceStorage.h"

#include <Preferences.h>

Preferences preferences;

static const char *NAMESPACE = "plantbed";

void beginDeviceStorage()
{
    preferences.begin(NAMESPACE, false);
}

StoredDeviceConfig loadStoredDeviceConfig()
{
    StoredDeviceConfig config;

    config.wifiSsid = preferences.getString("wifi_ssid", "");
    config.wifiPassword = preferences.getString("wifi_pass", "");

    config.hasWifiCredentials = config.wifiSsid.length() > 0;

    Serial.println();
    Serial.println("Loading stored device config...");

    if (config.hasWifiCredentials)
    {
        Serial.println("Stored Wi-Fi credentials found.");
        Serial.print("Stored SSID: ");
        Serial.println(config.wifiSsid);
    }
    else
    {
        Serial.println("No stored Wi-Fi credentials found.");
    }

    return config;
}

bool saveWifiCredentials(const String &ssid, const String &password)
{
    if (ssid.length() == 0)
    {
        Serial.println("Cannot save Wi-Fi credentials: SSID is empty.");
        return false;
    }

    preferences.putString("wifi_ssid", ssid);
    preferences.putString("wifi_pass", password);

    Serial.println("Wi-Fi credentials saved to flash.");

    return true;
}

void clearStoredDeviceConfig()
{
    preferences.clear();
    Serial.println("Stored device config cleared.");
}