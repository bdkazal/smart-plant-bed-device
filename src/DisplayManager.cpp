#include "DisplayManager.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "ApiClient.h"
#include "AppConfig.h"
#include "BootLogo.h"
#include "PinConfig.h"
#include "TimeSync.h"
#include "ValveController.h"
#include "WiFiMan.h"

Adafruit_SSD1306 oled(OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

static const int DISPLAY_TEXT_COLUMNS = 20;

bool displayAvailable = false;
bool displayAwake = false;
bool criticalDisplayActive = false;
unsigned long displaySleepAt = 0;

SensorReading latestDisplayReading;
bool hasLatestDisplayReading = false;

void clearAndPrepareText()
{
    oled.clearDisplay();
    oled.drawRect(0, 0, OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, SSD1306_WHITE);
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, 0);
}

// Four readable rows on a 128x64 OLED.
// Text is inset slightly so it does not touch the simple 1px border.
void printDisplayRow(int row, const String &text)
{
    oled.setCursor(4, row * 16 + 4);
    oled.print(text);
}

String limitText(String text, int maxLength)
{
    if (text.length() <= maxLength)
    {
        return text;
    }

    return text.substring(0, maxLength);
}

String centerText(String text)
{
    text = limitText(text, DISPLAY_TEXT_COLUMNS);

    int totalPadding = DISPLAY_TEXT_COLUMNS - text.length();
    int leftPadding = totalPadding / 2;

    String result = "";

    for (int i = 0; i < leftPadding; i++)
    {
        result += " ";
    }

    result += text;

    return result;
}

String leftRightText(String left, String right)
{
    left = limitText(left, DISPLAY_TEXT_COLUMNS);
    right = limitText(right, DISPLAY_TEXT_COLUMNS);

    int spaces = DISPLAY_TEXT_COLUMNS - left.length() - right.length();

    if (spaces < 1)
    {
        spaces = 1;
    }

    String result = left;

    for (int i = 0; i < spaces; i++)
    {
        result += " ";
    }

    result += right;

    return limitText(result, DISPLAY_TEXT_COLUMNS);
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

String soilValueText()
{
    if (!hasLatestDisplayReading || !latestDisplayReading.hasSoilMoisture)
    {
        return "SOIL --";
    }

    return "SOIL " + String(latestDisplayReading.soilMoisturePercent) + "%";
}

String soilStatusValueText()
{
    if (!hasLatestDisplayReading || !latestDisplayReading.hasSoilMoisture)
    {
        return "N/A";
    }

    if (latestDisplayReading.soilMoisturePercent <= SOIL_CRITICAL_PERCENT)
    {
        return "DRY";
    }

    if (deviceConfig.soilMoistureThreshold > 0 && latestDisplayReading.soilMoisturePercent <= deviceConfig.soilMoistureThreshold)
    {
        return "LOW";
    }

    return "OK";
}

String temperatureText()
{
    if (hasLatestDisplayReading && latestDisplayReading.hasTemperature)
    {
        return "Temp " + String((int)round(latestDisplayReading.temperatureC)) + "C";
    }

    return "Temp --C";
}

String humidityText()
{
    if (hasLatestDisplayReading && latestDisplayReading.hasHumidity)
    {
        return "Hum " + String((int)round(latestDisplayReading.humidityPercent)) + "%";
    }

    return "Hum --%";
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

    return "|| Plant Buddy ||";
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
    printDisplayRow(0, centerText("Plant Bed"));
    printDisplayRow(1, centerText("OLED ready"));
    oled.display();

    wakeDisplay(OLED_BOOT_SHOW_MS);
}

void displayShowBootLogo(unsigned long visibleMs)
{
    if (!displayAvailable)
    {
        return;
    }

    criticalDisplayActive = false;
    wakeDisplay(visibleMs);

    oled.clearDisplay();

    int x = (OLED_SCREEN_WIDTH - BOOT_LOGO_WIDTH) / 2;
    int y = (OLED_SCREEN_HEIGHT - BOOT_LOGO_HEIGHT) / 2;

    oled.drawBitmap(
        x,
        y,
        bootLogoBitmap,
        BOOT_LOGO_WIDTH,
        BOOT_LOGO_HEIGHT,
        SSD1306_WHITE);

    oled.display();
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
    printDisplayRow(0, centerText("BOOTING"));
    printDisplayRow(1, centerText(line1));
    printDisplayRow(2, centerText(line2));
    printDisplayRow(3, centerText(line3));
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
    printDisplayRow(0, centerText(statusTitleText()));
    printDisplayRow(1, leftRightText(modeText(), wateringStateText()));
    printDisplayRow(2, leftRightText(soilValueText(), soilStatusValueText()));
    printDisplayRow(3, leftRightText(temperatureText(), humidityText()));
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
    printDisplayRow(0, centerText("WATERING"));
    printDisplayRow(1, leftRightText(modeText(), wateringStateText()));
    printDisplayRow(2, leftRightText(soilValueText(), soilStatusValueText()));
    printDisplayRow(3, leftRightText("TIME", String(getWateringDurationSeconds()) + " sec"));
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
    printDisplayRow(0, centerText("Watering DONE"));
    printDisplayRow(1, leftRightText("VALVE", "CLOSED"));
    printDisplayRow(2, leftRightText(soilValueText(), soilStatusValueText()));
    printDisplayRow(3, centerText("Returning idle"));
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
    printDisplayRow(0, centerText("* DRY SOIL *"));
    printDisplayRow(1, leftRightText(soilValueText(), soilStatusValueText()));
    printDisplayRow(2, leftRightText("LIMIT", String(SOIL_CRITICAL_PERCENT) + "%"));
    printDisplayRow(3, leftRightText(modeText(), wateringStateText()));
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
