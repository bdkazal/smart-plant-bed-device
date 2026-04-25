#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ===== Wi-Fi config =====
const char *WIFI_SSID = "Andromeda";
const char *WIFI_PASSWORD = "W!f!P@55w@rD";

// ===== Laravel API config =====
// Use your Mac LAN IP, not localhost.
const char *API_BASE_URL = "http://192.168.0.102:8000";

// From Laravel devices table
const char *DEVICE_UUID = "1a6168e5-49d5-4e1b-8ff8-287e283d7b03";
const char *DEVICE_API_KEY = "gyVrsgLLUS86h6BNo8gRrmn2eDxETKtiR1tPgARd";

// Backend contract says heartbeat every 10–15 seconds.
// We will use 15 seconds.
const unsigned long HEARTBEAT_INTERVAL_MS = 15000;

unsigned long lastHeartbeatAt = 0;

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
    Serial.print("ESP32 IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength RSSI: ");
    Serial.println(WiFi.RSSI());
  }
  else
  {
    Serial.println("Wi-Fi connection failed.");
  }
}

void sendHeartbeat()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Cannot send heartbeat: Wi-Fi is not connected.");
    return;
  }

  String url = String(API_BASE_URL) + "/api/device/heartbeat";

  String body = "{";
  body += "\"device_uuid\":\"";
  body += DEVICE_UUID;
  body += "\"";
  body += "}";

  HTTPClient http;

  Serial.println();
  Serial.println("Sending heartbeat...");
  Serial.print("URL: ");
  Serial.println(url);
  Serial.print("Body: ");
  Serial.println(body);

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-DEVICE-KEY", DEVICE_API_KEY);

  int statusCode = http.POST(body);
  String response = http.getString();

  Serial.print("HTTP status: ");
  Serial.println(statusCode);

  Serial.print("Response: ");
  Serial.println(response);

  http.end();
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Smart Plant Bed ESP32 starting...");

  connectToWiFi();

  if (WiFi.status() == WL_CONNECTED)
  {
    sendHeartbeat();
    lastHeartbeatAt = millis();
  }
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Wi-Fi disconnected. Reconnecting...");
    connectToWiFi();
  }

  unsigned long now = millis();

  if (now - lastHeartbeatAt >= HEARTBEAT_INTERVAL_MS)
  {
    sendHeartbeat();
    lastHeartbeatAt = now;
  }
}