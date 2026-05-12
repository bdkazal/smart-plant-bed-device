#include "DisplayManager.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "AppConfig.h"
#include "PinConfig.h"
#include "TimeSync.h"
#include "ValveController.h"

extern Adafruit_SSD1306 oled;
extern bool displayAvailable;
extern bool displayAwake;
extern bool criticalDisplayActive;
extern unsigned long displaySleepAt;

static const int DISPLAY_TEXT_COLUMNS = 20;
static bool showSchedulePageNext = false;

String limitScheduleText(String text, int maxLength)
{
    if (text.length() <= maxLength)
    {
        return text;
    }

    return text.substring(0, maxLength);
}

String centerScheduleText(String text)
{
    text = limitScheduleText(text, DISPLAY_TEXT_COLUMNS);

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

String leftRightScheduleText(String left, String right)
{
    left = limitScheduleText(left, DISPLAY_TEXT_COLUMNS);
    right = limitScheduleText(right, DISPLAY_TEXT_COLUMNS);

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

    return limitScheduleText(result, DISPLAY_TEXT_COLUMNS);
}

void prepareSchedulePageText()
{
    oled.clearDisplay();
    oled.drawRect(0, 0, OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, SSD1306_WHITE);
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
}

void printSchedulePageRow(int row, const String &text)
{
    oled.setCursor(4, row * 16 + 4);
    oled.print(text);
}

void wakeSchedulePageDisplay(unsigned long visibleMs)
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

bool hasEnabledSchedule()
{
    if (!deviceConfig.hasLoadedConfig || deviceConfig.scheduleCount <= 0)
    {
        return false;
    }

    for (int i = 0; i < deviceConfig.scheduleCount; i++)
    {
        if (deviceConfig.schedules[i].isEnabled)
        {
            return true;
        }
    }

    return false;
}

String scheduleEnabledText()
{
    bool enabled = deviceConfig.wateringMode == "schedule" && hasEnabledSchedule();

    return enabled ? "YES" : "NO";
}

String currentTimeDisplayText()
{
    if (!isTimeReady())
    {
        return "NOT SET";
    }

    String currentTime = getCurrentHourMinuteString();

    if (currentTime.length() == 0)
    {
        return "NOT SET";
    }

    return currentTime;
}

int minutesOfDay(const String &timeText)
{
    if (timeText.length() < 5)
    {
        return -1;
    }

    int hour = timeText.substring(0, 2).toInt();
    int minute = timeText.substring(3, 5).toInt();

    if (hour < 0 || hour > 23 || minute < 0 || minute > 59)
    {
        return -1;
    }

    return hour * 60 + minute;
}

String dayShortName(int dayOfWeekIso)
{
    switch (dayOfWeekIso)
    {
    case 1:
        return "Mon";
    case 2:
        return "Tue";
    case 3:
        return "Wed";
    case 4:
        return "Thu";
    case 5:
        return "Fri";
    case 6:
        return "Sat";
    case 7:
        return "Sun";
    default:
        return "--";
    }
}

String nextScheduleDisplayText()
{
    if (deviceConfig.wateringMode != "schedule" || !hasEnabledSchedule())
    {
        return "--";
    }

    if (!isTimeReady())
    {
        return "--";
    }

    int currentDay = getCurrentDayOfWeekIso();
    String currentTime = getCurrentHourMinuteString();

    if (currentDay == 0 || currentTime.length() < 5)
    {
        return "--";
    }

    int currentMinutes = minutesOfDay(currentTime);

    if (currentMinutes < 0)
    {
        return "--";
    }

    int bestDeltaDays = 8;
    int bestMinutes = 24 * 60;
    int bestDay = 0;
    String bestTime = "";

    for (int i = 0; i < deviceConfig.scheduleCount; i++)
    {
        WateringScheduleConfig schedule = deviceConfig.schedules[i];

        if (!schedule.isEnabled)
        {
            continue;
        }

        int scheduleMinutes = minutesOfDay(schedule.timeOfDay);

        if (scheduleMinutes < 0)
        {
            continue;
        }

        int deltaDays = schedule.dayOfWeek - currentDay;

        if (deltaDays < 0)
        {
            deltaDays += 7;
        }

        if (deltaDays == 0 && scheduleMinutes <= currentMinutes)
        {
            deltaDays = 7;
        }

        if (deltaDays < bestDeltaDays || (deltaDays == bestDeltaDays && scheduleMinutes < bestMinutes))
        {
            bestDeltaDays = deltaDays;
            bestMinutes = scheduleMinutes;
            bestDay = schedule.dayOfWeek;
            bestTime = schedule.timeOfDay.substring(0, 5);
        }
    }

    if (bestTime.length() == 0)
    {
        return "--";
    }

    if (bestDeltaDays == 0)
    {
        return "Today " + bestTime;
    }

    return dayShortName(bestDay) + " " + bestTime;
}

void displayShowScheduleStatus(unsigned long visibleMs)
{
    if (!displayAvailable)
    {
        return;
    }

    criticalDisplayActive = false;
    wakeSchedulePageDisplay(visibleMs);

    prepareSchedulePageText();
    printSchedulePageRow(0, centerScheduleText("Schedule"));
    printSchedulePageRow(1, leftRightScheduleText("ENABLED", scheduleEnabledText()));
    printSchedulePageRow(2, leftRightScheduleText("TIME", currentTimeDisplayText()));
    printSchedulePageRow(3, leftRightScheduleText("NEXT", nextScheduleDisplayText()));
    oled.display();
}

void displayShowNextStatusPage(unsigned long visibleMs)
{
    if (criticalDisplayActive)
    {
        displayShowCriticalIfNeeded();
        return;
    }

    if (isWateringActive())
    {
        displayShowWateringStatus(visibleMs);
        return;
    }

    if (!showSchedulePageNext)
    {
        showSchedulePageNext = true;
        displayShowCurrentStatus(visibleMs);
        return;
    }

    showSchedulePageNext = false;
    displayShowScheduleStatus(visibleMs);
}
