#include "DisplayManager.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "ApiClient.h"
#include "AppConfig.h"
#include "PinConfig.h"
#include "TimeSync.h"
#include "ValveController.h"
#include "WiFiMan.h"

Adafruit_SSD1306 oled(OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

bool displayAvailable = false;
bool displayAwake = false;
bool criticalDisplayActive = false;
unsigned long displaySleepAt = 0;

SensorReading latestDisplayReading;
bool hasLatestDisplayReading = false;

void clearAndPrepareText()
{
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, 0);
}

void printLine(int line, const String &text)
{
    oled.setCursor(0, line * 8);
    oled.print(text);
}

String modeText()
{
    if (deviceConfig.wateringMode == "schedule")
    {
        return "SCHEDULE";
    }

    if (deviceConfig.wateringMode == "auto")
    {
        return "AUTO";
    }

    if (deviceConfig.wateringMode.length() == 0)
    {
        return "--";
    }

    return deviceConfig.wateringMode;
}

String wateringStateText()
{
    return isWateringActive() ? "WATER" : "IDLE";
}

String soilStatusText()
{
    if (!hasLatestDisplayReading || !latestDisplayReading.hasSoilMoisture)
    {
        return "Soil --";
    }

    String status = "OK";

    if (latestDisplayReading.soilMoisturePercent <= SOIL_CRITICAL_PERCENT)
    {
        status = "DRY";
    }
    else if (deviceConfig.soilMoistureThreshold > 0 && latestDisplayReading.soilMoisturePercent <= deviceConfig.soilMoistureThreshold)
    {
        status = "LOW";
    }

    return "Soil " + String(latestDisplayReading.soilMoisturePercent) + "% " + status;
}

String temperatureHumidityText()
{
    String temperature = "--";
    String humidity = "--";

    if (hasLatestDisplayReading && latestDisplayReading.hasTemperature)
    {
        temperature = String((int)round(latestDisplayReading.temperatureC));
    }

    if (hasLatestDisplayReading && latestDisplayReading.hasHumidity)
    {
        humidity = String((int)round(latestDisplayReading.humidityPercent));
    }

    return "T " + temperature + "C H " + humidity + "%";
}

String statusTitleText()
{
    if (!isWiFiConnected())
    {
        return "WIFI OFF";
    }

    if (!isServerRecentlyReachable())
    {
        return "LOCAL MODE";
    }

    return "Plant Bed";
}

void wakeDisplay(unsigned long visibleMs)
{
    if (!displayAvailable)
    {
        return;
    }

    oled.ssd1306_command(SSD1306_DISPLAYON);
    displayAwake = true;

    if (visibleMs > 0)
    {
        displaySleepAt = millis() + visibleMs;
    }
    else
    {
        displaySleepAt = 0;
    }
}

void sleepDisplay()
{
    if (!displayAvailable || !displayAwake)
    {
        return;
    }

    oled.clearDisplay();
    oled.display();
    oled.ssd1306_command(SSD1306_DISPLAYOFF);
    displayAwake = false;
    displaySleepAt = 0;
}

void beginDisplayManager()
{
    Wire.begin(OLED_I2C_SDA_PIN, OLED_I2C_SCL_PIN);

    displayAvailable = oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS);

    Serial.println();
    Serial.println("OLED display manager initialized.");
    Serial.print("OLED available: ");
    Serial.println(displayAvailable ? "yes" : "no");
    Serial.print("OLED I2C address: 0x");
    Serial.println(OLED_I2C_ADDRESS, HEX);

    if (!displayAvailable)
    {
        return;
    }

    clearAndPrepareText();
    printLine(0, "Smart Plant Bed");
    printLine(1, "OLED ready");
    oled.display();

    wakeDisplay(OLED_BOOT_SHOW_MS);
}

void displayShowBootStatus(const String &line1, const String &line2, const String &line3)
{
    if (!displayAvailable)
    {
        return;
    }

    criticalDisplayActive = false;
    wakeDisplay(OLED_BOOT_SHOW_MS);

    clearAndPrepareText();
    printLine(0, "BOOTING...");
    printLine(1, line1);
    printLine(2, line2);
    printLine(3, line3);
    oled.display();
}

void displaySetLatestSensorReading(const SensorReading &reading)
{
    latestDisplayReading = reading;
    hasLatestDisplayReading = true;
}

void displayShowCurrentStatus(unsigned long visibleMs)
{
    if (!displayAvailable)
    {
        return;
    }

    criticalDisplayActive = false;
    wakeDisplay(visibleMs);

    clearAndPrepareText();
    printLine(0, statusTitleText());
    printLine(1, modeText() + " " + wateringStateText());
    printLine(2, soilStatusText());
    printLine(3, temperatureHumidityText());
    oled.display();
}

void displayShowWateringStatus(unsigned long visibleMs)
{
    if (!displayAvailable)
    {
        return;
    }

    criticalDisplayActive = false;
    wakeDisplay(visibleMs);

    clearAndPrepareText();
    printLine(0, modeText() + " WATERING");
    printLine(1, soilStatusText());
    printLine(2, String(getWateringDurationSeconds()) + " sec");
    printLine(3, temperatureHumidityText());
    oled.display();
}

void displayShowWateringDone(unsigned long visibleMs)
{
    if (!displayAvailable)
    {
        return;
    }

    criticalDisplayActive = false;
    wakeDisplay(visibleMs);

    clearAndPrepareText();
    printLine(0, "Watering DONE");
    printLine(1, "Valve CLOSED");
    printLine(2, soilStatusText());
    oled.display();
}

void displayShowCriticalIfNeeded()
{
    if (!displayAvailable || !hasLatestDisplayReading)
    {
        return;
    }

    bool criticalDry = latestDisplayReading.hasSoilMoisture && latestDisplayReading.soilMoisturePercent <= SOIL_CRITICAL_PERCENT;

    if (!criticalDry)
    {
        if (criticalDisplayActive)
        {
            criticalDisplayActive = false;
            displayShowCurrentStatus(OLED_STATUS_SHOW_MS);
        }

        return;
    }

    criticalDisplayActive = true;
    wakeDisplay(0);

    clearAndPrepareText();
    printLine(0, "DRY SOIL");
    printLine(1, "Soil " + String(latestDisplayReading.soilMoisturePercent) + "%");
    printLine(2, "Critical " + String(SOIL_CRITICAL_PERCENT) + "%");
    printLine(3, modeText() + " " + wateringStateText());
    oled.display();
}

void updateDisplayManager()
{
    if (!displayAvailable)
    {
        return;
    }

    if (criticalDisplayActive)
    {
        displayShowCriticalIfNeeded();
        return;
    }

    if (displayAwake && displaySleepAt > 0 && millis() >= displaySleepAt)
    {
        sleepDisplay();
    }
}
