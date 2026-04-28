#include "SetupPortal.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "DeviceStorage.h"

static const char *SETUP_AP_SSID = "PlantBed-Setup";
static const char *SETUP_AP_PASSWORD = "plantbed123";

WebServer setupServer(80);
bool setupPortalActive = false;

String htmlEscape(const String &value)
{
    String escaped = value;
    escaped.replace("&", "&amp;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");
    escaped.replace("\"", "&quot;");
    escaped.replace("'", "&#039;");
    return escaped;
}

String jsonEscape(const String &value)
{
    String escaped = value;
    escaped.replace("\\", "\\\\");
    escaped.replace("\"", "\\\"");
    escaped.replace("\n", "\\n");
    escaped.replace("\r", "\\r");
    return escaped;
}

String setupPageHtml()
{
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Smart Plant Bed Setup</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: #f3f4f6;
      padding: 20px;
    }
    .card {
      max-width: 440px;
      margin: 30px auto;
      background: white;
      padding: 24px;
      border-radius: 12px;
      box-shadow: 0 2px 10px rgba(0,0,0,.08);
    }
    h1 {
      font-size: 22px;
      margin-bottom: 8px;
    }
    p {
      color: #555;
      line-height: 1.4;
    }
    label {
      display: block;
      margin-top: 16px;
      font-weight: bold;
    }
    select, input {
      width: 100%;
      box-sizing: border-box;
      padding: 12px;
      margin-top: 6px;
      border: 1px solid #ccc;
      border-radius: 8px;
      font-size: 16px;
    }
    button {
      width: 100%;
      margin-top: 22px;
      padding: 12px;
      background: #2563eb;
      color: white;
      border: 0;
      border-radius: 8px;
      font-size: 16px;
      cursor: pointer;
    }
    button:disabled {
      background: #9ca3af;
      cursor: not-allowed;
    }
    .secondary {
      display: block;
      text-align: center;
      margin-top: 14px;
      color: #2563eb;
      text-decoration: none;
      font-size: 14px;
      cursor: pointer;
    }
    .alert {
      padding: 12px;
      border-radius: 8px;
      margin-bottom: 16px;
      font-size: 14px;
      display: none;
    }
    .error {
      background: #fee2e2;
      color: #991b1b;
    }
    .success {
      background: #dcfce7;
      color: #166534;
    }
    .info {
      background: #dbeafe;
      color: #1e40af;
    }
    .small {
      font-size: 13px;
      color: #666;
      margin-top: 16px;
    }
  </style>
</head>
<body>
  <div class="card">
    <h1>Smart Plant Bed Setup</h1>
    <p>Select your home Wi-Fi and enter the password. The device will test the connection before saving.</p>

    <div id="message" class="alert"></div>

    <form id="wifi-form">
      <label for="ssid">Wi-Fi Network</label>
      <select id="ssid" name="ssid" required>
        <option value="">Scanning Wi-Fi networks...</option>
      </select>

      <label for="password">Wi-Fi Password</label>
      <input id="password" name="password" type="password">

      <button id="submit-button" type="submit">Test, Save and Restart</button>
    </form>

    <a class="secondary" onclick="loadNetworks()">Refresh Wi-Fi List</a>

    <p class="small">
      Setup hotspot: PlantBed-Setup<br>
      Setup password: plantbed123<br>
      Setup page: http://192.168.4.1
    </p>
  </div>

  <script>
    const messageBox = document.getElementById('message');
    const ssidSelect = document.getElementById('ssid');
    const passwordInput = document.getElementById('password');
    const submitButton = document.getElementById('submit-button');
    const form = document.getElementById('wifi-form');

    function showMessage(text, type) {
      messageBox.textContent = text;
      messageBox.className = 'alert ' + type;
      messageBox.style.display = 'block';
    }

    function clearMessage() {
      messageBox.textContent = '';
      messageBox.style.display = 'none';
    }

    async function loadNetworks() {
      clearMessage();
      ssidSelect.innerHTML = '<option value="">Scanning Wi-Fi networks...</option>';

      try {
        const response = await fetch('/networks');
        const data = await response.json();

        ssidSelect.innerHTML = '';

        if (!data.networks || data.networks.length === 0) {
          ssidSelect.innerHTML = '<option value="">No Wi-Fi networks found</option>';
          showMessage('No Wi-Fi networks found. Move closer to your router and refresh.', 'error');
          return;
        }

        data.networks.forEach(network => {
          const option = document.createElement('option');
          option.value = network.ssid;
          option.textContent = network.ssid + ' (' + network.rssi + ' dBm)';
          ssidSelect.appendChild(option);
        });
      } catch (error) {
        ssidSelect.innerHTML = '<option value="">Failed to scan Wi-Fi</option>';
        showMessage('Could not scan Wi-Fi networks. Refresh and try again.', 'error');
      }
    }

    form.addEventListener('submit', async function (event) {
      event.preventDefault();

      const ssid = ssidSelect.value;
      const password = passwordInput.value;

      if (!ssid) {
        showMessage('Please select a Wi-Fi network.', 'error');
        return;
      }

      submitButton.disabled = true;
      showMessage('Testing Wi-Fi credentials. This may take up to 15 seconds...', 'info');

      const body = new URLSearchParams();
      body.append('ssid', ssid);
      body.append('password', password);

      try {
        const response = await fetch('/save', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/x-www-form-urlencoded'
          },
          body: body.toString()
        });

        const data = await response.json();

        if (!data.ok) {
          showMessage(data.message || 'Could not connect. Check the password and try again.', 'error');
          submitButton.disabled = false;
          return;
        }

        showMessage('Wi-Fi saved successfully. Device is restarting. The setup hotspot will disappear.', 'success');
        submitButton.disabled = true;
      } catch (error) {
        showMessage('Connection test failed. Try again.', 'error');
        submitButton.disabled = false;
      }
    });

    loadNetworks();
  </script>
</body>
</html>
)rawliteral";
}

void handleRoot()
{
    setupServer.send(200, "text/html", setupPageHtml());
}

void handleNetworks()
{
    Serial.println();
    Serial.println("Scanning Wi-Fi networks for setup page...");

    WiFi.mode(WIFI_AP_STA);

    int networkCount = WiFi.scanNetworks();

    String json = "{\"networks\":[";

    bool first = true;

    for (int i = 0; i < networkCount; i++)
    {
        String ssid = WiFi.SSID(i);
        int rssi = WiFi.RSSI(i);

        if (ssid.length() == 0)
        {
            continue;
        }

        if (!first)
        {
            json += ",";
        }

        json += "{";
        json += "\"ssid\":\"";
        json += jsonEscape(ssid);
        json += "\",";
        json += "\"rssi\":";
        json += String(rssi);
        json += "}";

        first = false;
    }

    json += "]}";

    WiFi.scanDelete();

    setupServer.send(200, "application/json", json);
}

bool testWiFiCredentials(const String &ssid, const String &password)
{
    Serial.println();
    Serial.println("Testing submitted Wi-Fi credentials...");
    Serial.print("SSID: ");
    Serial.println(ssid);

    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;

    while (WiFi.status() != WL_CONNECTED && attempts < 30)
    {
        delay(500);
        Serial.print(".");
        setupServer.handleClient();
        attempts++;
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("Submitted Wi-Fi credentials worked.");
        Serial.print("Temporary station IP: ");
        Serial.println(WiFi.localIP());
        return true;
    }

    Serial.println("Submitted Wi-Fi credentials failed.");

    WiFi.disconnect(false, false);
    WiFi.mode(WIFI_AP_STA);

    return false;
}

void handleSave()
{
    String ssid = setupServer.arg("ssid");
    String password = setupServer.arg("password");

    ssid.trim();

    if (ssid.length() == 0)
    {
        setupServer.send(
            400,
            "application/json",
            "{\"ok\":false,\"message\":\"Please select a Wi-Fi network.\"}");
        return;
    }

    bool connected = testWiFiCredentials(ssid, password);

    if (!connected)
    {
        setupServer.send(
            200,
            "application/json",
            "{\"ok\":false,\"message\":\"Could not connect to that Wi-Fi. Check the password and try again.\"}");
        return;
    }

    bool saved = saveWifiCredentials(ssid, password);

    if (!saved)
    {
        setupServer.send(
            500,
            "application/json",
            "{\"ok\":false,\"message\":\"Connection worked, but saving credentials failed.\"}");
        return;
    }

    setupServer.send(
        200,
        "application/json",
        "{\"ok\":true,\"message\":\"Wi-Fi saved successfully. Device is restarting.\"}");

    delay(1500);
    ESP.restart();
}

void handleNotFound()
{
    setupServer.sendHeader("Location", "/", true);
    setupServer.send(302, "text/plain", "");
}

void startSetupPortal()
{
    Serial.println();
    Serial.println("Starting setup portal...");

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(SETUP_AP_SSID, SETUP_AP_PASSWORD);

    IPAddress ip = WiFi.softAPIP();

    Serial.print("Setup hotspot SSID: ");
    Serial.println(SETUP_AP_SSID);
    Serial.print("Setup hotspot password: ");
    Serial.println(SETUP_AP_PASSWORD);
    Serial.print("Setup portal URL: http://");
    Serial.println(ip);

    setupServer.on("/", HTTP_GET, handleRoot);
    setupServer.on("/networks", HTTP_GET, handleNetworks);
    setupServer.on("/save", HTTP_POST, handleSave);
    setupServer.on("/favicon.ico", HTTP_GET, []()
                   { setupServer.send(204, "text/plain", ""); });
    setupServer.onNotFound(handleNotFound);

    setupServer.begin();

    setupPortalActive = true;

    Serial.println("Setup portal started.");
}

void handleSetupPortal()
{
    if (!setupPortalActive)
    {
        return;
    }

    setupServer.handleClient();
}

bool isSetupPortalActive()
{
    return setupPortalActive;
}