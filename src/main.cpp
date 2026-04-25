#include <Arduino.h>
#include <WiFi.h>

// Temporary test credentials.
// Later we will move these to a safer secrets/config file.
const char *WIFI_SSID = "Andromeda";
const char *WIFI_PASSWORD = "W!f!P@55w@rD";

void connectToWiFi()
{
  Serial.println();
  Serial.println("Connecting to Wi-Fi...");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 30)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Wi-Fi connected successfully.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength RSSI: ");
    Serial.println(WiFi.RSSI());
  }
  else
  {
    Serial.println("Wi-Fi connection failed.");
    Serial.println("Check SSID/password and make sure the network is 2.4GHz.");
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Smart Plant Bed ESP32 starting...");

  connectToWiFi();
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Wi-Fi is still connected.");
  }
  else
  {
    Serial.println("Wi-Fi disconnected. Trying again...");
    connectToWiFi();
  }

  delay(5000);
}