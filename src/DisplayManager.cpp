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

// Four readable rows on a 128x64 OLED.
// Each row gets 8px text height + about 8px empty space below it.
void printDisplayRow(int row, const String &text)
{
    oled.setCursor(0, row * 16);
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
    return isWateringActive() ? "Watering" : "IDLE";
}

String paddedModeStateText()
{
    String mode = modeText();
    String state = wateringStateText();

    while (mode.length() < 10)
    {
        mode += " ";
    }

    return mode + state;
}

String soilStatusText()
{
    if (!hasLatestDisplayReading || !latestDisplayReading.hasSoilMoisture)
    {
        return "SOIL  --    N/A";
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

    String percent = String(latestDisplayReading.soilMoisturePercent) + "%";

    while (percent.length() < 5)
    {
        percent += " ";
    }

    return "SOIL  " + percent + " " + status;
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

    return "T " + temperature + "C     H " + humidity + "%";
}

String statusTitleText()
{
    if (!isWiFiConnected())
    {
        return "WIFI OFFLINE";
    }

    if (!isServerRecentlyReachable())
    {
        return "Offline Mode";
    }

    return "SMART PLANT BED";
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
    printDisplayRow(0, "SMART PLANT BED");
    printDisplayRow(1, "OLED ready");
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
    printDisplayRow(0, "BOOTING...");
    printDisplayRow(1, line1);
    printDisplayRow(2, line2);
    printDisplayRow(3, line3);
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
    printDisplayRow(0, statusTitleText());
    printDisplayRow(1, paddedModeStateText());
    printDisplayRow(2, soilStatusText());
    printDisplayRow(3, temperatureHumidityText());
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
    printDisplayRow(0, "WATERING");
    printDisplayRow(1, paddedModeStateText());
    printDisplayRow(2, soilStatusText());
    printDisplayRow(3, "TIME  " + String(getWateringDurationSeconds()) + " sec");
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
    printDisplayRow(0, "WATERING DONE");
    printDisplayRow(1, "VALVE     CLOSED");
    printDisplayRow(2, soilStatusText());
    printDisplayRow(3, "Returning idle");
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
    printDisplayRow(0, "* DRY SOIL *");
    printDisplayRow(1, "SOIL  " + String(latestDisplayReading.soilMoisturePercent) + "%");
    printDisplayRow(2, "LIMIT " + String(SOIL_CRITICAL_PERCENT) + "%");
    printDisplayRow(3, paddedModeStateText());
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
