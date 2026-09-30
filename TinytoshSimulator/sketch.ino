
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/Picopixel.h>
#include <OneButton.h>
#include <Wire.h>
#include <time.h>

#include "images.h"
#include "structs.h"

Adafruit_SSD1306 display(128, 64, &Wire, -1);
OneButton button;

AppState appState;

int currentScreen = 0;
int currentSubScreen = 0;
unsigned long lastScreenSwitch = 0;

uint8_t screenBufferOld[1024];
uint8_t screenBufferNew[1024];

const int CONTRAST_DIM = 1;
const int CONTRAST_MAX = 255;

String getCurrentTimeShort(String format) {
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char time_str[12];
    if (format == "12") strftime(time_str, sizeof(time_str), "%I:%M", &timeinfo);
    else strftime(time_str, sizeof(time_str), "%H:%M", &timeinfo);
    return String(time_str);
}

String getFullDate() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return "No Date";
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%A, %b %d", &timeinfo);
    return String(buffer);
}

String formatMinsFromMidnight(int mins, String format, bool show_ampm = true) {
    if (mins == -1) return "--:--";
    int h = mins / 60;
    int m = mins % 60;
    char buf[12];
    if (format == "24") {
        snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
    } else {
        int displayH = h % 12;
        if (displayH == 0) displayH = 12;
        if (show_ampm) {
            const char* ampm = (h >= 12) ? "PM" : "AM";
            snprintf(buf, sizeof(buf), "%02d:%02d %s", displayH, m, ampm);
        } else {
            snprintf(buf, sizeof(buf), "%02d:%02d", displayH, m);
        }
    }
    return String(buf);
}

String formatDurationMins(int mins) {
    if (mins == -1) return "--";
    char buf[12];
    snprintf(buf, sizeof(buf), "%dh %dm", mins / 60, mins % 60);
    return String(buf);
}

String getWeatherDescription(int wmo_code) {
    if (wmo_code == 0) return "Clear Sky";
    if (wmo_code >= 1 && wmo_code <= 3) return "Cloudy";
    if (wmo_code >= 45 && wmo_code <= 48) return "Fog";
    if (wmo_code >= 51 && wmo_code <= 67) return "Rain";
    if (wmo_code >= 71 && wmo_code <= 77) return "Snow";
    if (wmo_code >= 95) return "Thunder";
    return "Unknown";
}

long long getLivePopulation(long long basePop, double growth, int year) {
    if (basePop <= 0 || year <= 0) return basePop;

    struct tm timeinfo = {0};
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = 0;
    timeinfo.tm_mday = 1;

    time_t baseline = mktime(&timeinfo);
    time_t now = time(nullptr);

    if (now < baseline) return basePop;

    double growth_per_sec = (basePop * (growth / 100.0)) / 31557600.0;
    double diff_sec = difftime(now, baseline);

    return basePop + (long long)(diff_sec * growth_per_sec);
}

const unsigned char* getWeatherBitmap(int wmo_code, bool is_day) {
    if (wmo_code == 0) {
        return is_day ? icon_sun : icon_moon;
    } else if (wmo_code >= 1 && wmo_code <= 3) {
        return is_day ? icon_cloud : icon_cloud_moon;
    } else if (wmo_code >= 45 && wmo_code <= 48) return icon_fog;
    else if (wmo_code >= 51 && wmo_code <= 67) return icon_rain;
    else if (wmo_code >= 71 && wmo_code <= 77) return icon_snow;
    else if (wmo_code >= 95) return icon_thunder;
    return icon_cloud;
}

const unsigned char* getAQIBitmap(int val, bool is_eu) {
    if (is_eu) {
        if (val <= 20) return icon_smile;
        if (val <= 60) return icon_neutral;
        if (val <= 80) return icon_bad;
        return icon_dead;
    } else {
        if (val <= 50)  return icon_smile;
        if (val <= 100) return icon_neutral;
        if (val <= 150) return icon_bad;
        return icon_dead;
    }
}

void drawInfoScreen(const unsigned char* image = nullptr, String text = "No Data") {
    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setFont();

    display.drawRect(1, 1, 126, 62, 1);
    display.drawRect(3, 3, 122, 58, 1);

    int16_t x1, y1;
    uint16_t w, h;

    if (image != nullptr) {
        display.setTextSize(1);

        display.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
        int textX = (128 - w) / 2;

        display.drawBitmap(48, 10, image, 32, 32, 1);
        display.setCursor(textX, 46);
        display.print(text);
    } else {
        display.setTextSize(2);

        display.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
        int textX = (128 - w) / 2;
        int textY = (64 - h) / 2;

        display.setCursor(textX, textY);
        display.print(text);
    }
}

void drawTimeScreen(const Config& config, String timeStr, String dateStr) {
    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setFont();

    int16_t x1, y1;
    uint16_t w, h;

    if (config.date_display) {
        display.setTextSize(3);
        display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor((128 - w) / 2, 10);
        display.print(timeStr);

        display.setTextSize(1);
        display.getTextBounds(dateStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor((128 - w) / 2, 48);
        display.print(dateStr);
    } else {
        display.setTextSize(4);
        display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);

        int xPos = (128 - w) / 2;
        int yPos = (64 - h) / 2 - y1;

        display.setCursor(xPos, yPos);
        display.print(timeStr);
    }
}

void drawCalendarScreen(const Config& config, const CalendarData& calendar) {
    display.clearDisplay();

    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    int day = timeinfo.tm_mday;
    int mon = timeinfo.tm_mon;
    int year = timeinfo.tm_year + 1900;
    int wday = timeinfo.tm_wday;

    const char* months[] = {"JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE", "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"};
    const char* weekdays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

    String todayHolidayName = "";
    if (config.calendar_show_holidays) {
        char todayBuf[16];
        snprintf(todayBuf, sizeof(todayBuf), "%04d-%02d-%02d", year, mon + 1, day);
        String todayStr = String(todayBuf);
        for (int i = 0; i < calendar.count; i++) {
            if (calendar.items[i].date == todayStr) {
                todayHolidayName = calendar.items[i].name;
                break;
            }
        }
    }

    int colWidth = config.calendar_minimal ? 128 : 57;

    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setFont();

    int16_t x1, y1; uint16_t w, h;

    display.setTextSize(3);
    display.getTextBounds(String(day).c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor((colWidth - w) / 2, 3);
    display.print(day);

    display.setTextSize(1);
    display.getTextBounds(months[mon], 0, 0, &x1, &y1, &w, &h);
    display.setCursor((colWidth - w) / 2, 29);
    display.print(months[mon]);

    display.getTextBounds(String(year).c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor((colWidth - w) / 2, 53);
    display.print(year);

    if (todayHolidayName != "") {
        display.setFont(&Picopixel);

        String lines[2] = {"", ""};
        int l = 0, start = 0;
        todayHolidayName.toUpperCase();

        while (start < todayHolidayName.length() && l < 2) {
            int spaceIdx = todayHolidayName.indexOf(' ', start);
            if (spaceIdx == -1) spaceIdx = todayHolidayName.length();
            String word = todayHolidayName.substring(start, spaceIdx);

            String testLine = lines[l].length() == 0 ? word : lines[l] + " " + word;
            display.getTextBounds(testLine.c_str(), 0, 0, &x1, &y1, &w, &h);

            if (w > colWidth) {
                if (lines[l].length() == 0) { lines[l] = word; l++; }
                else { l++; if (l < 2) lines[l] = word; }
            } else {
                lines[l] = testLine;
            }
            start = spaceIdx + 1;
        }

        int numLines = (lines[1].length() > 0) ? 2 : 1;
        int cursorY = (numLines == 1) ? 47 : 44;

        for (int i = 0; i < numLines; i++) {
            if (lines[i].length() > 0) {
                display.getTextBounds(lines[i].c_str(), 0, 0, &x1, &y1, &w, &h);
                bool truncated = false;
                while (w > colWidth && lines[i].length() > 0) {
                    lines[i] = lines[i].substring(0, lines[i].length() - 1);
                    display.getTextBounds((lines[i] + ".").c_str(), 0, 0, &x1, &y1, &w, &h);
                    truncated = true;
                }
                if (truncated) lines[i] += ".";

                display.setCursor((colWidth - w) / 2, cursorY);
                display.print(lines[i]);
                cursorY += 6;
            }
        }
        display.setFont();
    } else {
        display.getTextBounds(weekdays[wday], 0, 0, &x1, &y1, &w, &h);
        display.setCursor((colWidth - w) / 2, 41);
        display.print(weekdays[wday]);
    }

    if (config.calendar_minimal) return;

    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) daysInMonth[1] = 29;
    int numDays = daysInMonth[mon];

    int firstWday = (wday - ((day - 1) % 7) + 7) % 7;

    bool isMondayFirst = (config.calendar_start_day == "mon");
    int startCol = isMondayFirst ? ((firstWday + 6) % 7) : firstWday;

    int totalCells = startCol + numDays;
    int numLines = (totalCells + 6) / 7;

    display.setFont(&Picopixel);

    const char* headersMon[] = {"M", "T", "W", "T", "F", "S", "S"};
    const char* headersSun[] = {"S", "M", "T", "W", "T", "F", "S"};
    const char** headers = isMondayFirst ? headersMon : headersSun;

    int colCenters[] = {62, 72, 82, 92, 102, 112, 122};

    for (int i = 0; i < 7; i++) {
        display.getTextBounds(headers[i], 0, 0, &x1, &y1, &w, &h);
        display.setCursor(colCenters[i] - (w / 2), 6);
        display.print(headers[i]);
    }

    int lineY = (numLines == 6) ? 8 : 9;
    display.drawLine(59, lineY, 126, lineY, SSD1306_WHITE);

    int rowY = (numLines == 6) ? 16 : 18;
    int rowSpacing = (numLines == 6) ? 9 : 10;

    int currCol = startCol;

    for (int d = 1; d <= numDays; d++) {
        char dateBuf[16];
        snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d", year, mon + 1, d);
        String dateStr = String(dateBuf);

        bool isHoliday = false;
        if (config.calendar_show_holidays) {
            for (int i = 0; i < calendar.count; i++) {
                if (calendar.items[i].date == dateStr) {
                    isHoliday = true;
                    break;
                }
            }
        }

        int xCenter = colCenters[currCol];

        if (isHoliday) {
            display.fillRect(xCenter - 4, rowY - 5, 9, 7, SSD1306_WHITE);
            display.setTextColor(SSD1306_BLACK);
        } else {
            display.setTextColor(SSD1306_WHITE);
        }

        String dStr = String(d);
        display.getTextBounds(dStr.c_str(), 0, 0, &x1, &y1, &w, &h);
        display.setCursor(xCenter - (w / 2), rowY);
        display.print(dStr);

        display.setTextColor(SSD1306_WHITE);

        if (d == day) {
            display.drawRect(xCenter - 5, rowY - 6, 11, 9, SSD1306_WHITE);
        }

        currCol++;
        if (currCol > 6) {
            currCol = 0;
            rowY += rowSpacing;
        }
    }
}

void drawWeatherScreen(const Config& config, const WeatherData& data, const String& currentTime) {
    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setFont();

    int16_t x1, y1;
    uint16_t w, h;

    bool valid = !isnan(data.temp) && data.weather_code != -1;

    bool showHeader = config.weather_show_header;

    float displayTemp = data.temp;
    if (config.temp_unit == "F") displayTemp = displayTemp * 1.8 + 32;

    String weatherKeys[6];
    int weatherKeyCount = 0;
    for (int i = 0; i < 6; i++) {
        if (config.weather_values[i].length() > 0) weatherKeys[weatherKeyCount++] = config.weather_values[i];
    }

    auto weatherIconFor = [&](const String& key) -> const unsigned char* {
        if (key == "feels") return icon_feel;
        if (key == "humidity") return icon_drop;
        if (key == "wind") return icon_wind;
        if (key == "precipitation") return icon_precipitation;
        if (key == "pressure") return icon_pressure;
        if (key == "visibility") return icon_visibility;
        return icon_drop;
    };

    auto weatherTextFor = [&](const String& key) -> String {
        if (!valid) return "--";
        if (key == "feels") {
            float displayApparent = data.apparent_temperature;
            if (config.temp_unit == "F") displayApparent = displayApparent * 1.8 + 32;
            return config.round_temps ? String((int)round(displayApparent)) : String(displayApparent, 1);
        }
        if (key == "humidity") return String(data.humidity) + "%";
        if (key == "wind") return String((int)round(data.wind_speed)) + "km";
        if (key == "precipitation") return isnan(data.precipitation_probability) ? "--" : String((int)round(data.precipitation_probability)) + "%";
        if (key == "pressure") return isnan(data.pressure) ? "--" : String((int)round(data.pressure)) + "hPa";
        if (key == "visibility") return isnan(data.visibility) ? "--" : String(data.visibility / 1000.0, 1) + "km";
        return "--";
    };

    auto drawWeatherRow = [&](int count, int y) {
        for (int i = 0; i < count; i++) {
            const String& key = weatherKeys[i];
            int xIcon = 5 + i * 43;
            display.drawBitmap(xIcon, y, weatherIconFor(key), 8, 8, SSD1306_WHITE);
            String val = weatherTextFor(key);
            int xVal = xIcon + 10;
            display.setCursor(xVal, y + 1);
            display.print(val);
            if (key == "feels") {
                display.getTextBounds(val.c_str(), 0, 0, &x1, &y1, &w, &h);
                display.drawBitmap(xVal + w, y + 1, degree_icon_small, 4, 4, SSD1306_WHITE);
            }
        }
    };

    auto drawWeatherList = [&]() {
        int margin = 3;
        int rowH = 8;
        int xIcon = display.width() - margin - rowH;
        float gap = (float)(display.height() - 2 * margin - weatherKeyCount * rowH) / (weatherKeyCount + 1);
        for (int i = 0; i < weatherKeyCount; i++) {
            int y = (int)round(margin + gap * (i + 1) + rowH * i);
            const String& key = weatherKeys[i];
            String val = weatherTextFor(key);
            display.getTextBounds(val.c_str(), 0, 0, &x1, &y1, &w, &h);
            bool hasDegree = (key == "feels");
            int contentW = (int)w + (hasDegree ? 4 : 0);
            int xText = xIcon - 5 - contentW;
            display.setCursor(xText, y + 1);
            display.print(val);
            if (hasDegree) {
                display.drawBitmap(xText + w, y + 1, degree_icon_small, 4, 4, SSD1306_WHITE);
            }
            display.drawBitmap(xIcon, y, weatherIconFor(key), 8, 8, SSD1306_WHITE);
        }
    };

    if (showHeader) {

        display.setTextSize(1);
        String cityStr = valid ? config.city : "No Location";

        int yHeader = 2;

        display.getTextBounds(currentTime.c_str(), 0, 0, &x1, &y1, &w, &h);
        int xTime = display.width() - w - 2;

        int cityMaxWidth = xTime - 2 - 3;
        display.getTextBounds(cityStr.c_str(), 0, 0, &x1, &y1, &w, &h);
        bool cityTrimmed = false;
        if ((int)w > cityMaxWidth) {
            for (int i = cityStr.length(); i > 0; i--) {
                String trunc = cityStr.substring(0, i);
                display.getTextBounds(trunc.c_str(), 0, 0, &x1, &y1, &w, &h);
                if ((int)w + 8 <= cityMaxWidth) {
                    cityStr = trunc;
                    cityTrimmed = true;
                    break;
                }
            }
        }
        display.setCursor(2, yHeader);
        display.print(cityStr);
        if (cityTrimmed) {
            display.drawBitmap(2 + w, yHeader, icon_dots, 8, 8, SSD1306_WHITE);
        }

        display.setCursor(xTime, yHeader);
        display.print(currentTime);

        int ySeparator = 14;
        display.drawFastHLine(0, ySeparator, display.width(), SSD1306_WHITE);

        int yMiddleStart = 18;
        int middleHeight = 35;

        display.setTextSize(3);

        String tempValueStr = valid ? (config.round_temps ? String((int)round(displayTemp)) : String(displayTemp, 1)) : "--";

        display.getTextBounds(tempValueStr.c_str(), 0, 0, &x1, &y1, &w, &h);
        int yTemp = yMiddleStart + ((middleHeight - h) / 2);
        int xStartTemp = 5;

        display.setCursor(xStartTemp, yTemp);
        display.print(tempValueStr);

        int xDegree = xStartTemp + w + 2;
        display.drawBitmap(xDegree, yTemp, degree_icon, 12, 12, SSD1306_WHITE);

        int iconSize = 24;
        int rightMargin = 5;
        int xRightEdge = display.width() - rightMargin;

        int xIcon = xRightEdge - iconSize;
        int yIcon = yMiddleStart + 1;

        const unsigned char* iconBitmap = valid ? getWeatherBitmap(data.weather_code, data.is_day) : icon_cloud;
        display.drawBitmap(xIcon, yIcon, iconBitmap, iconSize, iconSize, SSD1306_WHITE);

        display.setTextSize(1);
        String desc = valid ? getWeatherDescription(data.weather_code) : "No Data";
        display.getTextBounds(desc.c_str(), 0, 0, &x1, &y1, &w, &h);

        int xDesc = xRightEdge - w;
        int yDesc = yIcon + iconSize + 1;
        display.setCursor(xDesc, yDesc);
        display.print(desc);

        int yFooter = 56;
        int count = weatherKeyCount > 3 ? 3 : weatherKeyCount;
        drawWeatherRow(count, yFooter);
    } else {

        int numberX = 3;
        int topY = 3;

        display.setTextSize(3);
        String tempValueStr = valid ? (config.round_temps ? String((int)round(displayTemp)) : String(displayTemp, 1)) : "--";
        display.getTextBounds(tempValueStr.c_str(), 0, 0, &x1, &y1, &w, &h);
        display.setCursor(numberX, topY);
        display.print(tempValueStr);

        int degreeSize = 12;
        display.drawBitmap(numberX + (int)w + 2, topY, degree_icon, degreeSize, degreeSize, SSD1306_WHITE);

        int numberW = (int)w;
        int iconSize = 24;
        int iconY = topY + h + 5;
        int iconX = numberX + (numberW - iconSize) / 2;
        const unsigned char* iconBitmap = valid ? getWeatherBitmap(data.weather_code, data.is_day) : icon_cloud;
        display.drawBitmap(iconX, iconY, iconBitmap, iconSize, iconSize, SSD1306_WHITE);

        display.setTextSize(1);
        display.setFont(&Picopixel);
        String desc = valid ? getWeatherDescription(data.weather_code) : "No Data";
        desc.toUpperCase();
        display.getTextBounds(desc.c_str(), 0, 0, &x1, &y1, &w, &h);
        display.setCursor(numberX + (numberW - (int)w) / 2, iconY + iconSize + 2 - y1);
        display.print(desc);
        display.setFont();

        drawWeatherList();
    }
}

void drawAQIScreen(const Config& config, const AirQualityData& data, const String& currentTime) {
    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setFont();

    int16_t x1, y1;
    uint16_t w, h;

    bool valid = (data.aqi != -1);

    bool showHeader = config.aqi_show_header;

    String aqiKeys[6];
    int aqiKeyCount = 0;
    for (int i = 0; i < 6; i++) {
        if (config.aqi_values[i].length() > 0) aqiKeys[aqiKeyCount++] = config.aqi_values[i];
    }

    auto aqiIconFor = [&](const String& key) -> const unsigned char* {
        if (key == "pm25") return icon_small_particles;
        if (key == "pm10") return icon_big_particles;
        if (key == "no2") return icon_no2;
        if (key == "co") return icon_co;
        if (key == "co2") return icon_co2;
        if (key == "so2") return icon_so2;
        if (key == "o3") return icon_o3;
        if (key == "dust") return icon_dust;
        if (key == "uv") return icon_uv;
        if (key == "ch4") return icon_ch4;
        return icon_small_particles;
    };

    auto aqiHasUgUnit = [&](const String& key) -> bool {
        return key == "pm25" || key == "pm10" || key == "no2" || key == "co" || key == "so2" || key == "o3" || key == "dust";
    };

    auto aqiValueFor = [&](const String& key) -> float {
        if (key == "pm25") return data.pm25;
        if (key == "pm10") return data.pm10;
        if (key == "no2") return data.no2;
        if (key == "co") return data.co;
        if (key == "co2") return data.co2;
        if (key == "so2") return data.so2;
        if (key == "o3") return data.o3;
        if (key == "dust") return data.dust;
        if (key == "uv") return data.uv;
        if (key == "ch4") return data.ch4;
        return NAN;
    };

    auto drawAqiHeaderRow = [&](int position, const String& key, int y) {
        float rawVal = aqiValueFor(key);
        String val = (!valid || isnan(rawVal)) ? "--" : String((int)round(rawVal));
        bool hasUnit = aqiHasUgUnit(key);
        display.getTextBounds(val.c_str(), 0, 0, &x1, &y1, &w, &h);
        int totalWidth = 8 + 2 + w + (hasUnit ? 1 + 8 : 0);
        int xIcon;
        if (position == 0) xIcon = 2;
        else if (position == 1) xIcon = (display.width() / 2) - (totalWidth / 2);
        else xIcon = display.width() - totalWidth - 2;
        display.drawBitmap(xIcon, y, aqiIconFor(key), 8, 8, SSD1306_WHITE);
        int xText = xIcon + 8 + 2;
        display.setCursor(xText, y + 1);
        display.print(val);
        if (hasUnit) {
            display.drawBitmap(xText + w + 1, y + 1, icon_ug, 8, 8, SSD1306_WHITE);
        }
    };

    auto drawAqiList = [&]() {
        int margin = 3;
        int rowH = 8;
        int xIcon = display.width() - margin - rowH;
        float gap = (float)(display.height() - 2 * margin - aqiKeyCount * rowH) / (aqiKeyCount + 1);
        for (int i = 0; i < aqiKeyCount; i++) {
            int y = (int)round(margin + gap * (i + 1) + rowH * i);
            const String& key = aqiKeys[i];
            float rawVal = aqiValueFor(key);
            String val = (!valid || isnan(rawVal)) ? "--" : String((int)round(rawVal));
            bool hasUnit = aqiHasUgUnit(key);
            display.getTextBounds(val.c_str(), 0, 0, &x1, &y1, &w, &h);
            int xUnit = xIcon - 5 - 8;
            int textRightEdge = (hasUnit ? xUnit : xIcon) - 5;
            int xText = textRightEdge - w;
            display.setCursor(xText, y + 1);
            display.print(val);
            if (hasUnit) {
                display.drawBitmap(xUnit, y + 1, icon_ug, 8, 8, SSD1306_WHITE);
            }
            display.drawBitmap(xIcon, y, aqiIconFor(key), 8, 8, SSD1306_WHITE);
        }
    };

    if (showHeader) {

        display.setTextSize(1);
        String cityStr = valid ? config.city : "No Location";

        int yHeader = 2;

        display.getTextBounds(currentTime.c_str(), 0, 0, &x1, &y1, &w, &h);
        int xTime = display.width() - w - 2;

        int cityMaxWidth = xTime - 2 - 3;
        display.getTextBounds(cityStr.c_str(), 0, 0, &x1, &y1, &w, &h);
        bool cityTrimmed = false;
        if ((int)w > cityMaxWidth) {
            for (int i = cityStr.length(); i > 0; i--) {
                String trunc = cityStr.substring(0, i);
                display.getTextBounds(trunc.c_str(), 0, 0, &x1, &y1, &w, &h);
                if ((int)w + 8 <= cityMaxWidth) {
                    cityStr = trunc;
                    cityTrimmed = true;
                    break;
                }
            }
        }
        display.setCursor(2, yHeader);
        display.print(cityStr);
        if (cityTrimmed) {
            display.drawBitmap(2 + w, yHeader, icon_dots, 8, 8, SSD1306_WHITE);
        }

        display.setCursor(xTime, yHeader);
        display.print(currentTime);

        int ySeparator = 14;
        display.drawFastHLine(0, ySeparator, display.width(), SSD1306_WHITE);

        int yMiddleStart = 18;
        int middleHeight = 35;

        display.setTextSize(3);
        String aqiStr = valid ? String(data.aqi) : "--";

        display.getTextBounds(aqiStr.c_str(), 0, 0, &x1, &y1, &w, &h);
        int yAqi = yMiddleStart + ((middleHeight - h) / 2);
        int xStartAqi = 5;

        display.setCursor(xStartAqi, yAqi);
        display.print(aqiStr);

        display.setTextSize(1);
        int xLabel = xStartAqi + w + 4;

        display.setCursor(xLabel, yAqi);
        display.print(config.aqi_type);

        display.setCursor(xLabel, yAqi + 9);
        display.print("AQI");

        int iconSize = 24;
        int rightMargin = 5;
        int xRightEdge = display.width() - rightMargin;
        int xIcon = xRightEdge - iconSize;
        int yIcon = yMiddleStart + 1;

        const unsigned char* aqiIcon = valid ? getAQIBitmap(data.aqi, config.aqi_type == "EU") : icon_neutral;
        display.drawBitmap(xIcon, yIcon, aqiIcon, iconSize, iconSize, SSD1306_WHITE);

        display.setTextSize(1);
        String desc = valid ? data.status : "No Data";
        display.getTextBounds(desc.c_str(), 0, 0, &x1, &y1, &w, &h);

        int xDesc = xRightEdge - w;
        int yDesc = yIcon + iconSize + 1;
        display.setCursor(xDesc, yDesc);
        display.print(desc);

        int yFooter = 56;
        int count = aqiKeyCount > 3 ? 3 : aqiKeyCount;
        for (int i = 0; i < count; i++) drawAqiHeaderRow(i, aqiKeys[i], yFooter);
    } else {

        int numberX = 3;
        int topY = 3;

        String aqiStr = valid ? String(data.aqi) : "--";

        display.setTextSize(3);
        display.getTextBounds(aqiStr.c_str(), 0, 0, &x1, &y1, &w, &h);
        display.setCursor(numberX, topY);
        display.print(aqiStr);

        display.setTextSize(1);
        int xLabel = numberX + (int)w + 4;
        display.setCursor(xLabel, topY);
        display.print(config.aqi_type);
        display.setCursor(xLabel, topY + 9);
        display.print("AQI");

        int numberW = (int)w;
        int iconSize = 24;
        int iconY = topY + h + 5;
        int iconX = numberX + (numberW - iconSize) / 2;
        const unsigned char* aqiIcon = valid ? getAQIBitmap(data.aqi, config.aqi_type == "EU") : icon_neutral;
        display.drawBitmap(iconX, iconY, aqiIcon, iconSize, iconSize, SSD1306_WHITE);

        display.setTextSize(1);
        display.setFont(&Picopixel);
        String desc = valid ? data.status : "No Data";
        desc.toUpperCase();
        display.getTextBounds(desc.c_str(), 0, 0, &x1, &y1, &w, &h);
        display.setCursor(numberX + (numberW - (int)w) / 2, iconY + iconSize + 2 - y1);
        display.print(desc);
        display.setFont();

        drawAqiList();
    }
}

void drawDaylightScreen(const Config& config, const DaylightData& data) {
    if (data.sunrise_mins == -1) {
        drawInfoScreen(icon_error, "No Daylight Data");
        return;
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);

    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    int currentMins = timeinfo.tm_hour * 60 + timeinfo.tm_min;

    bool isDay = (currentMins >= data.sunrise_mins && currentMins < data.sunset_mins);
    int progress = 0;
    int mins_left = 0;

    if (isDay) {
        int total_daylight = data.sunset_mins - data.sunrise_mins;
        int daylight_passed = currentMins - data.sunrise_mins;
        if (total_daylight > 0) progress = (daylight_passed * 100) / total_daylight;
        mins_left = data.sunset_mins - currentMins;
    } else {
        int total_night = (1440 - data.sunset_mins) + data.sunrise_mins;
        int night_passed = (currentMins >= data.sunset_mins) ?
                           (currentMins - data.sunset_mins) :
                           ((1440 - data.sunset_mins) + currentMins);

        if (total_night > 0) progress = (night_passed * 100) / total_night;
        mins_left = total_night - night_passed;
    }

    int leftMins = isDay ? data.sunrise_mins : data.sunset_mins;
    int rightMins = isDay ? data.sunset_mins : data.sunrise_mins;
    int centerMins = isDay ? data.noon_mins : (data.noon_mins + 720) % 1440;
    int displayLengthMins = isDay ? data.length_mins : (1440 - data.length_mins);

    const unsigned char* leftIcon = isDay ? icon_sun_rise : icon_sun_set;
    const unsigned char* rightIcon = isDay ? icon_sun_set : icon_sun_rise;
    const unsigned char* centerIcon = isDay ? icon_sun : icon_moon;

    int iconY = config.daylight_minimal ? 14 : 2;
    int textY = iconY + 24 + 4;

    int16_t x1, y1; uint16_t w, h;

    display.drawBitmap(19 - 12, iconY, leftIcon, 24, 24, 1);
    display.drawBitmap(64 - 12, iconY, centerIcon, 24, 24, 1);
    display.drawBitmap(110 - 12, iconY, rightIcon, 24, 24, 1);

    display.setFont();
    display.setTextSize(1);

    String leftStr = formatMinsFromMidnight(leftMins, config.time_format, false);
    display.getTextBounds(leftStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(19 - (w / 2), textY);
    display.print(leftStr);

    String centerStr = formatMinsFromMidnight(centerMins, config.time_format, false);
    display.getTextBounds(centerStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(64 - (w / 2), textY);
    display.print(centerStr);

    String rightStr = formatMinsFromMidnight(rightMins, config.time_format, false);
    display.getTextBounds(rightStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(110 - (w / 2), textY);
    display.print(rightStr);

    if (config.daylight_minimal) {
        return;
    }

    display.drawRect(2, 45, 124, 6, 1);
    int fillW = (int)((constrain(progress, 0, 100) / 100.0) * 120);
    if (fillW > 0) display.fillRect(4, 47, fillW, 2, 1);

    String progStr = String(progress) + "%";
    display.getTextBounds(progStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(64 - (w / 2), 55);
    display.print(progStr);

    String lengthStr = formatDurationMins(displayLengthMins);
    lengthStr.replace(" ", "");
    display.setCursor(2, 55);
    display.print(lengthStr);

    String timeLeftStr = String(mins_left / 60) + "h" + String(mins_left % 60) + "m";
    display.getTextBounds(timeLeftStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(126 - w, 55);
    display.print(timeLeftStr);
}

void drawMoonScreen(const Config& config, const MoonData& data) {
    if (data.fracillum == -1) {
        drawInfoScreen(icon_error, "No Moon Data");
        return;
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setFont();

    int16_t x1, y1; uint16_t w, h;
    bool isMinimal = config.moon_minimal;

    String phaseStr = data.curphase;
    phaseStr.toUpperCase();
    display.getTextBounds(phaseStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 55);
    display.print(phaseStr);

    int cx = 64;
    int cy = 31;
    int r = 17;

    if (!isMinimal) {
        String illumStr = String(data.fracillum) + "%";
        display.getTextBounds(illumStr.c_str(), 0, 0, &x1, &y1, &w, &h);
        display.setCursor((128 - w) / 2, 2);
        display.print(illumStr);

        String riseStr = (data.rise_mins != -1) ? formatMinsFromMidnight(data.rise_mins, config.time_format) : "--:--";
        display.setCursor(6, 29);
        display.print(riseStr);
        display.drawBitmap(17, 21, icon_up, 7, 4, 1);

        String setStr = (data.set_mins != -1) ? formatMinsFromMidnight(data.set_mins, config.time_format) : "--:--";
        display.setCursor(92, 29);
        display.print(setStr);
        display.drawBitmap(103, 21, icon_down, 7, 4, 1);
    } else {
        cy = 27;
        r = 22;
    }

    float f = data.fracillum / 100.0;

    bool isWaxing = (data.curphase.indexOf("Wax") >= 0 || data.curphase.indexOf("First") >= 0 || data.curphase.indexOf("New") >= 0);

    display.fillCircle(cx, cy, r, SSD1306_WHITE);

    if (isWaxing) {
        display.fillRect(cx - r, cy - r, r, 2 * r + 1, SSD1306_BLACK);
    } else {
        display.fillRect(cx, cy - r, r + 1, 2 * r + 1, SSD1306_BLACK);
    }

    int ew = round(r * fabsf(1.0f - 2.0f * f));

    int ellipseColor = (f <= 0.5) ? SSD1306_BLACK : SSD1306_WHITE;

    for (int y = -r; y <= r; y++) {
        int dx = (ew * sqrt(r * r - y * y)) / r;
        display.drawFastHLine(cx - dx, cy + y, dx * 2 + 1, ellipseColor);
    }

    display.drawCircle(cx, cy, r, SSD1306_WHITE);
}

void drawPopulationScreen(const Config& config, const PopulationData& data) {
    if (data.world_pop_base == -1 && data.country_pop_base == -1) {
        drawInfoScreen(icon_error, "No Populace Data");
        return;
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setFont();

    int16_t x1, y1; uint16_t w, h;
    bool showW = config.pop_show_world && data.world_year > 0;
    bool showC = config.pop_show_country && data.country_year > 0;

    auto formatPop = [](long long num) -> String {
        String str = String(num);
        int p = str.length() - 3;
        while (p > 0) { str = str.substring(0, p) + "," + str.substring(p); p -= 3; }
        return str;
    };

    if (showW && showC) {
        long long liveW = getLivePopulation(data.world_pop_base, data.world_growth, data.world_year);
        long long liveC = getLivePopulation(data.country_pop_base, data.country_growth, data.country_year);

        display.drawBitmap(8, 4, icon_world, 16, 16, 1);
        display.getTextBounds("World", 0, 0, &x1, &y1, &w, &h);
        display.setCursor(16 - (w / 2), 21);
        display.print("World");

        String cCode = config.country_code;
        cCode.toUpperCase();
        if (cCode.length() > 2) cCode = cCode.substring(0, 2);

        display.drawBitmap(9, 36, icon_location, 13, 16, 1);
        display.getTextBounds(cCode.c_str(), 0, 0, &x1, &y1, &w, &h);
        display.setCursor(16 - (w / 2), 53);
        display.print(cCode);

        String popWStr = formatPop(liveW);
        display.getTextBounds(popWStr.c_str(), 0, 0, &x1, &y1, &w, &h);

        int w_world_pop = w;
        int world_pop_x = 126 - w_world_pop;
        int world_icon_x = world_pop_x - 12;
        int center_ref = world_icon_x + (12 + w_world_pop) / 2;

        display.drawBitmap(world_icon_x, 6, icon_people, 8, 8, 1);
        display.setCursor(world_pop_x, 7);
        display.print(popWStr);

        String grWStr = (data.world_growth > 0 ? "+" : "") + String(data.world_growth, 2) + "%";
        display.getTextBounds(grWStr.c_str(), 0, 0, &x1, &y1, &w, &h);
        int gr_w_icon_x = center_ref - (12 + w) / 2;
        display.drawBitmap(gr_w_icon_x, 18, (data.world_growth >= 0 ? icon_up_arrow : icon_down_arrow), 8, 8, 1);
        display.setCursor(gr_w_icon_x + 12, 19);
        display.print(grWStr);

        String popCStr = formatPop(liveC);
        display.getTextBounds(popCStr.c_str(), 0, 0, &x1, &y1, &w, &h);
        int ctr_pop_icon_x = center_ref - (12 + w) / 2;
        display.drawBitmap(ctr_pop_icon_x, 38, icon_people, 8, 8, 1);
        display.setCursor(ctr_pop_icon_x + 12, 39);
        display.print(popCStr);

        String grCStr = (data.country_growth > 0 ? "+" : "") + String(data.country_growth, 2) + "%";
        display.getTextBounds(grCStr.c_str(), 0, 0, &x1, &y1, &w, &h);
        int gr_c_icon_x = center_ref - (12 + w) / 2;
        display.drawBitmap(gr_c_icon_x, 50, (data.country_growth >= 0 ? icon_up_arrow : icon_down_arrow), 8, 8, 1);
        display.setCursor(gr_c_icon_x + 12, 51);
        display.print(grCStr);

    } else if (showW || showC) {
        long long base = showW ? data.world_pop_base : data.country_pop_base;
        double growth = showW ? data.world_growth : data.country_growth;
        int year = showW ? data.world_year : data.country_year;

        long long liveP = getLivePopulation(base, growth, year);
        String title = showW ? "World" : config.country;

        display.getTextBounds(title.c_str(), 0, 0, &x1, &y1, &w, &h);
        if (w > 128) {
            String trunc;
            for (int i = title.length(); i > 0; i--) {
                trunc = title.substring(0, i) + "...";
                display.getTextBounds(trunc.c_str(), 0, 0, &x1, &y1, &w, &h);
                if (w <= 128) {
                    title = trunc;
                    break;
                }
            }
        }

        const unsigned char* mainIcon = showW ? icon_world : icon_location;
        int iconW = showW ? 16 : 13;

        display.drawBitmap((128 - iconW) / 2, 6, mainIcon, iconW, 16, 1);

        display.getTextBounds(title.c_str(), 0, 0, &x1, &y1, &w, &h);
        display.setCursor((128 - w) / 2, 23);
        display.print(title);

        String popStr = formatPop(liveP);
        display.getTextBounds(popStr.c_str(), 0, 0, &x1, &y1, &w, &h);
        int popBlockW = 12 + w;
        int popIconX = (128 - popBlockW) / 2;

        display.drawBitmap(popIconX, 36, icon_people, 8, 8, 1);
        display.setCursor(popIconX + 12, 37);
        display.print(popStr);

        String grStr = (growth > 0 ? "+" : "") + String(growth, 2) + "%";
        display.getTextBounds(grStr.c_str(), 0, 0, &x1, &y1, &w, &h);
        int grBlockW = 12 + w;
        int grIconX = (128 - grBlockW) / 2;

        display.drawBitmap(grIconX, 50, (growth >= 0 ? icon_up_arrow : icon_down_arrow), 8, 8, 1);
        display.setCursor(grIconX + 12, 51);
        display.print(grStr);
    }
}

void drawCryptoScreen(const Config& config, const CryptoData& data) {
    if (!data.updated) {
        drawInfoScreen(icon_error, "No Crypto Data");
        return;
    }

    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setFont();

    display.setTextSize(3);
    display.setCursor(4, 6);
    display.print(data.symbol);

    if (config.crypto_fn) {
        display.setTextSize(1);
        display.setCursor(4, 32);
        String displayName = data.name;
        int maxLen = 20;
        if (displayName.length() > maxLen) displayName = displayName.substring(0, maxLen - 3) + "...";
        displayName.toUpperCase();
        display.print(displayName);
    }

    display.setTextSize(2);
    display.setCursor(4, 44);
    display.print("$" + String(data.price_usd));

    bool isPositive = (data.percent_change_24h >= 0);
    const unsigned char* arrowIcon = isPositive ? icon_arrow_up : icon_arrow_down;
    display.drawBitmap(102, 3, arrowIcon, 15, 15, 1);

    display.setTextSize(1);
    display.setCursor(95, 22);
    String trendPrefix = isPositive ? "+" : "";
    display.print(trendPrefix + String(data.percent_change_24h, 1) + "%");
}

void drawCurrencyScreen(const Config& config, const CurrencyData& data, int multiplier) {
    if (!data.updated) {
        drawInfoScreen(icon_error, "No Currency Data");
        return;
    }

    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setFont();

    display.setTextSize(3);
    display.setCursor(4, 6);
    display.print(data.base);

    if (config.currency_fn) {
        String fullName = "Unknown";
        for (auto c : allCurrencies) {
            if (data.base.equalsIgnoreCase(c.code)) {
                fullName = c.name;
                break;
            }
        }
        display.setTextSize(1);
        display.setCursor(4, 32);
        int maxLen = 20;
        if (fullName.length() > maxLen) fullName = fullName.substring(0, maxLen - 3) + "...";
        fullName.toUpperCase();
        display.print(fullName);
    }

    float displayRate = data.rate * multiplier;
    int decimals = (displayRate < 10.0) ? 3 : (displayRate < 100.0) ? 2 : (displayRate < 1000.0) ? 1 : 0;

    display.setTextSize(2);
    display.setCursor(4, 44);
    display.print(String(displayRate, decimals) + " " + data.target);

    display.setTextSize(1);
    String topText = String(multiplier) + " " + data.base;
    int16_t x1, y1; uint16_t wTop, hTop;
    display.getTextBounds(topText, 0, 0, &x1, &y1, &wTop, &hTop);
    int topTextX = 128 - wTop - 4;
    display.setCursor(topTextX, 8);
    display.print(topText);

    String eqText = "=";
    uint16_t wEq, hEq;
    display.getTextBounds(eqText, 0, 0, &x1, &y1, &wEq, &hEq);
    int centerOfTopText = topTextX + (wTop / 2);
    display.setCursor(centerOfTopText - (wEq / 2), 19);
    display.print(eqText);
}

void drawStockScreen(const Config& config, const StockData& data) {
    if (!data.updated) {
        drawInfoScreen(icon_error, "No Stock Data");
        return;
    }

    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setFont();

    display.setTextSize(3);
    display.setCursor(4, 6);
    display.print(data.symbol);

    if (config.stock_fn) {
        display.setTextSize(1);
        display.setCursor(4, 32);
        String displayName = data.name;
        int maxLen = 20;
        if (displayName.length() > maxLen) displayName = displayName.substring(0, maxLen - 3) + "...";
        displayName.toUpperCase();
        display.print(displayName);
    }

    display.setTextSize(2);
    display.setCursor(4, 44);
    display.print("$" + String(data.price));

    bool isPositive = (data.percent_change >= 0);
    const unsigned char* arrowIcon = isPositive ? icon_arrow_up : icon_arrow_down;
    display.drawBitmap(102, 3, arrowIcon, 15, 15, 1);

    display.setTextSize(1);
    display.setCursor(95, 22);
    String trendPrefix = isPositive ? "+" : "";
    display.print(trendPrefix + String(data.percent_change, 1) + "%");
}

void drawPcScreen(const PcStats& pcStats) {
    bool isInvalid = (isnan(pcStats.cpu_percent) || pcStats.cpu_percent == 0) && (isnan(pcStats.mem_percent) || pcStats.mem_percent == 0);

    if (isInvalid) {
        drawInfoScreen(icon_monitor, "No PC");
        return;
    }

    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setFont();

    const int BAR_X = 20;
    const int BAR_W = 86;
    const int BAR_H = 6;

    const int FILL_X_OFFSET = 2;
    const int FILL_Y_OFFSET = 2;
    const int MAX_FILL_W = BAR_W - 4;
    const int FILL_H = 2;
    const int TEXT_X = 110;

    auto drawInfilledBar = [&](int y, float percent) {
        display.drawRect(BAR_X, y, BAR_W, BAR_H, 1);
        int fillW = (int)((constrain(percent, 0, 100) / 100.0) * MAX_FILL_W);
        if (fillW > 0) {
            display.fillRect(BAR_X + FILL_X_OFFSET, y + FILL_Y_OFFSET, fillW, FILL_H, 1);
        }
    };

    display.drawBitmap(0, 0, icon_cpu_percent, 16, 16, 1);
    drawInfilledBar(5, pcStats.cpu_percent);
    display.setCursor(TEXT_X, 4);
    display.print(String((int)round(pcStats.cpu_percent)) + "%");

    display.drawBitmap(0, 16, icon_ram_percent, 16, 16, 1);
    drawInfilledBar(21, pcStats.mem_percent);
    display.setCursor(TEXT_X, 20);
    display.print(String((int)round(pcStats.mem_percent)) + "%");

    display.drawBitmap(0, 32, icon_disk_percent, 16, 16, 1);
    drawInfilledBar(37, pcStats.disk_percent);
    display.setCursor(TEXT_X, 36);
    display.print(String((int)round(pcStats.disk_percent)) + "%");

    display.drawBitmap(0, 48, icon_net_down, 16, 16, 1);

    float netPercent = (pcStats.net_down_kb / 5120.0) * 100.0;
    drawInfilledBar(53, netPercent);

    display.setCursor(TEXT_X, 52);

    if (pcStats.net_down_kb >= 1024) {
        display.print(String((int)round(pcStats.net_down_kb / 1024.0)) + "M");
    } else if (pcStats.net_down_kb >= 100) {
        display.print("<1M");
    } else {
        display.print(String((int)pcStats.net_down_kb) + "K");
    }
}

void drawMediaScreen(const PcMedia& media) {
    bool isInvalid = (media.status.length() == 0 || media.name.length() == 0 || media.author.length() == 0 || media.name.equalsIgnoreCase("Unknown"));

    if (isInvalid) {
        drawInfoScreen(icon_note, "No Media");
        return;
    }

    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setFont();

    display.drawBitmap(2, 4, icon_note, 32, 32, 1);

    display.setFont(&Picopixel);
    String statusStr = media.status;
    statusStr.toUpperCase();
    if (statusStr == "") statusStr = "STOPPED";

    int16_t x1, y1; uint16_t w, h;
    display.getTextBounds(statusStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(18 - (w / 2), 46);
    display.print(statusStr);

    const unsigned char* iconBits = icon_stop;
    if (statusStr == "PLAYING") iconBits = icon_play;
    if (statusStr == "PAUSED")  iconBits = icon_pause;
    display.drawBitmap(14, 52, iconBits, 8, 8, 1);

    auto drawSmartText = [&](String text, int x, int &y, const GFXfont* font, bool isPicopixel, int maxLines) {
        if (text == "") return;
        display.setFont(font);

        String lines[8] = {"", "", "", "", "", "", "", ""};
        int lineCount = 0;
        int start = 0;
        int maxWidth = 82;

        while (start < text.length()) {
            int spaceIdx = text.indexOf(' ', start);
            if (spaceIdx == -1) spaceIdx = text.length();
            String word = text.substring(start, spaceIdx);

            String testLine = lines[lineCount].length() == 0 ? word : lines[lineCount] + " " + word;
            display.getTextBounds(testLine.c_str(), 0, 0, &x1, &y1, &w, &h);

            if (w > maxWidth) {
                if (lines[lineCount].length() == 0) {
                    lines[lineCount++] = word;
                } else {
                    lineCount++;
                    if (lineCount < maxLines) lines[lineCount] = word;
                }
                if (lineCount == maxLines) break;
            } else {
                lines[lineCount] = testLine;
            }
            start = spaceIdx + 1;
        }
        if (lineCount < maxLines && lines[lineCount].length() > 0) lineCount++;

        if (start < text.length() && lineCount == maxLines) {
            String& lastLine = lines[maxLines - 1];
            while (lastLine.length() > 0) {
                display.getTextBounds((lastLine + "...").c_str(), 0, 0, &x1, &y1, &w, &h);
                if (w <= maxWidth) break;
                int lastSpace = lastLine.lastIndexOf(' ');
                if (lastSpace == -1) lastLine = lastLine.substring(0, lastLine.length() - 1);
                else lastLine = lastLine.substring(0, lastSpace);
            }
            lastLine += "...";
        }

        for (int i = 0; i < lineCount; i++) {
            if (isPicopixel) {
                y += 5;
                display.setCursor(x, y);
                display.print(lines[i]);
                y += 1;
            } else {
                display.setCursor(x, y);
                display.print(lines[i]);
                y += 8 + 1;
            }
        }
        y += 6;
    };

    int cursorY = 4;

    String trackName = media.name;
    trackName.toUpperCase();

    String albumName = media.album;
    albumName.toUpperCase();
    bool hasAlbum = (albumName.length() > 0 && albumName != "UNKNOWN");

    int reservedForAuthor = 15;
    int reservedForAlbum = hasAlbum ? 12 : 0;

    int trackAvailY = 64 - cursorY - reservedForAuthor - reservedForAlbum;
    int maxTrackLines = max(1, trackAvailY / 9);
    drawSmartText(trackName, 44, cursorY, nullptr, false, maxTrackLines);

    int authorAvailY = 64 - cursorY - reservedForAlbum;
    int maxAuthorLines = max(1, authorAvailY / 9);
    drawSmartText(media.author, 44, cursorY, nullptr, false, maxAuthorLines);

    if (hasAlbum) {
        int albumAvailY = 64 - cursorY;
        int maxAlbumLines = max(1, albumAvailY / 6);
        drawSmartText(albumName, 44, cursorY, &Picopixel, true, maxAlbumLines);
    }
}

void drawBambuScreen(const BambuData& data) {
    bool isInvalid = (data.status == "SYNCING" || data.status.length() == 0);

    if (isInvalid) {
        drawInfoScreen(icon_printer, "No Printer");
        return;
    }

    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setFont();

    String status = data.status;
    status.toUpperCase();
    bool isIdle = (status == "IDLE" || status == "FINISH" || status == "FINISHED" || status == "FAILED");

    if (isIdle) {
        display.drawBitmap(22, 9, icon_printer, 32, 32, 1);

        display.setTextSize(1);
        display.setCursor(26, 45);
        display.print("IDLE");

        display.setTextSize(2);

        String n_val = String((int)round(data.nozzle_temp));
        int n_circle_x = 79 + (n_val.length() * 12) + 5;

        display.drawBitmap(65, 17, icon_nozzle, 8, 8, 1);
        display.setCursor(79, 13);
        display.print(n_val);
        display.drawCircle(n_circle_x, 16, 3, SSD1306_WHITE);

        String b_val = String((int)round(data.bed_temp));
        int b_circle_x = 79 + (b_val.length() * 12) + 5;

        display.drawBitmap(65, 41, icon_bed, 8, 8, 1);
        display.setCursor(79, 37);
        display.print(b_val);
        display.drawCircle(b_circle_x, 40, 3, SSD1306_WHITE);

        return;
    }

    display.setTextSize(1);
    display.setFont();

    int16_t x1, y1;
    uint16_t w, h;

    display.drawBitmap(4, 4, icon_nozzle, 8, 8, 1);
    display.drawBitmap(4, 16, icon_bed, 8, 8, 1);
    display.drawBitmap(116, 4, icon_part_fan, 8, 8, 1);
    display.drawBitmap(116, 16, icon_aux_fan, 8, 8, 1);

    String n_lhs = String((int)round(data.nozzle_temp));
    String n_rhs = "/" + String((int)round(data.nozzle_target)) + " C";
    String b_lhs = String((int)round(data.bed_temp));
    String b_rhs = "/" + String((int)round(data.bed_target)) + " C";

    uint16_t w_n_lhs, w_b_lhs;
    display.getTextBounds(n_lhs.c_str(), 0, 0, &x1, &y1, &w_n_lhs, &h);
    display.getTextBounds(b_lhs.c_str(), 0, 0, &x1, &y1, &w_b_lhs, &h);

    int startX = 16;
    int x_slash = startX + max(w_n_lhs, w_b_lhs);

    display.setCursor(x_slash - w_n_lhs, 4);
    display.print(n_lhs + n_rhs);

    display.setCursor(x_slash - w_b_lhs, 16);
    display.print(b_lhs + b_rhs);

    String p_fan = String(data.fan_part) + "%";
    String a_fan = String(data.fan_aux) + "%";

    display.getTextBounds(p_fan.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(116 - 4 - w, 4);
    display.print(p_fan);

    display.getTextBounds(a_fan.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(116 - 4 - w, 16);
    display.print(a_fan);

    String fileName = data.file_name;

    if (fileName.length() == 0 || fileName.equalsIgnoreCase("None")) {
        fileName = "Idle";
    }

    int maxWidth = 120;
    display.getTextBounds(fileName.c_str(), 0, 0, &x1, &y1, &w, &h);

    if (w > maxWidth) {
        String trunc;
        for (int i = 1; i <= fileName.length() / 2; i++) {
            int leftLen = (fileName.length() / 2) - i;
            int rightLen = fileName.length() - (fileName.length() / 2) - i;
            trunc = fileName.substring(0, leftLen) + "..." + fileName.substring(fileName.length() - rightLen);

            display.getTextBounds(trunc.c_str(), 0, 0, &x1, &y1, &w, &h);
            if (w <= maxWidth) {
                fileName = trunc;
                break;
            }
        }
    }

    display.getTextBounds(fileName.c_str(), 0, 0, &x1, &y1, &w, &h);
    int cursorX = (128 - w) / 2;

    display.setCursor(cursorX, 31);
    display.print(fileName);

    display.drawRect(4, 43, 120, 6, 1);
    int fillW = (int)((constrain(data.progress, 0, 100) / 100.0) * 116);
    if (fillW > 0) display.fillRect(6, 45, fillW, 2, 1);

    String progStr = String(data.progress) + "%";
    String layerStr = String(data.layer) + "/" + String(data.total_layers);
    String timeStr = String(data.time_left) + "m";

    display.setCursor(4, 53);
    display.print(progStr);

    display.getTextBounds(layerStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(64 - (w / 2), 53);
    display.print(layerStr);

    display.getTextBounds(timeStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(128 - 4 - w, 53);
    display.print(timeStr);
}

void drawFlightScreen(const Config& config, const FlightData& data) {
    const float kmPerNauticalMile = 1.852f;

    auto formatAltitude = [&](float altitudeFt) -> String {
        if (isnan(altitudeFt)) return "--";
        if (config.flight_units == "metric") return String((int)round(altitudeFt * 0.3048)) + " m";
        return String((int)round(altitudeFt)) + " ft";
    };

    auto formatVelocity = [&](float velocityKt) -> String {
        if (isnan(velocityKt)) return "--";
        if (config.flight_units == "metric") return String((int)round(velocityKt * kmPerNauticalMile)) + " km/h";
        return String((int)round(velocityKt)) + " kt";
    };

    auto mapAccentedChar = [](uint32_t cp) -> String {
        switch (cp) {
            case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5:
            case 0x100: case 0x102: case 0x104: return "A";
            case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5:
            case 0x101: case 0x103: case 0x105: return "a";
            case 0xC7: case 0x106: case 0x108: case 0x10A: case 0x10C: return "C";
            case 0xE7: case 0x107: case 0x109: case 0x10B: case 0x10D: return "c";
            case 0xC8: case 0xC9: case 0xCA: case 0xCB:
            case 0x112: case 0x114: case 0x116: case 0x118: case 0x11A: return "E";
            case 0xE8: case 0xE9: case 0xEA: case 0xEB:
            case 0x113: case 0x115: case 0x117: case 0x119: case 0x11B: return "e";
            case 0xCC: case 0xCD: case 0xCE: case 0xCF:
            case 0x128: case 0x12A: case 0x12C: case 0x12E: case 0x130: return "I";
            case 0xEC: case 0xED: case 0xEE: case 0xEF:
            case 0x129: case 0x12B: case 0x12D: case 0x12F: case 0x131: return "i";
            case 0xD1: case 0x143: case 0x145: case 0x147: return "N";
            case 0xF1: case 0x144: case 0x146: case 0x148: return "n";
            case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6: case 0xD8:
            case 0x14C: case 0x14E: case 0x150: return "O";
            case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF6: case 0xF8:
            case 0x14D: case 0x14F: case 0x151: return "o";
            case 0xD9: case 0xDA: case 0xDB: case 0xDC:
            case 0x168: case 0x16A: case 0x16C: case 0x16E: case 0x170: case 0x172: return "U";
            case 0xF9: case 0xFA: case 0xFB: case 0xFC:
            case 0x169: case 0x16B: case 0x16D: case 0x16F: case 0x171: case 0x173: return "u";
            case 0xDD: case 0x176: case 0x178: return "Y";
            case 0xFD: case 0xFF: case 0x177: return "y";
            case 0xD0: return "D";
            case 0xF0: return "d";
            case 0xDE: return "Th";
            case 0xFE: return "th";
            case 0xDF: return "ss";
            case 0xC6: return "AE";
            case 0xE6: return "ae";
            case 0x152: return "OE";
            case 0x153: return "oe";
            case 0x160: return "S";
            case 0x161: return "s";
            case 0x179: case 0x17B: case 0x17D: return "Z";
            case 0x17A: case 0x17C: case 0x17E: return "z";
            case 0x141: return "L";
            case 0x142: return "l";
            case 0x158: return "R";
            case 0x159: return "r";
            case 0x164: return "T";
            case 0x165: return "t";
            case 0x11E: return "G";
            case 0x11F: return "g";
            default: return "";
        }
    };

    auto sanitizeAscii = [&](const String& input) -> String {
        String out;
        out.reserve(input.length());

        for (size_t i = 0; i < input.length();) {
            uint8_t c = (uint8_t)input[i];

            if (c < 0x80) {
                out += (char)c;
                i++;
                continue;
            }

            uint32_t codepoint = 0;
            int extraBytes;
            if ((c & 0xE0) == 0xC0) { codepoint = c & 0x1F; extraBytes = 1; }
            else if ((c & 0xF0) == 0xE0) { codepoint = c & 0x0F; extraBytes = 2; }
            else if ((c & 0xF8) == 0xF0) { codepoint = c & 0x07; extraBytes = 3; }
            else { i++; continue; }

            size_t seqLen = 1;
            bool valid = true;
            for (int b = 0; b < extraBytes; b++) {
                if (i + seqLen >= input.length()) { valid = false; break; }
                uint8_t cont = (uint8_t)input[i + seqLen];
                if ((cont & 0xC0) != 0x80) { valid = false; break; }
                codepoint = (codepoint << 6) | (cont & 0x3F);
                seqLen++;
            }
            if (!valid) { i++; continue; }

            out += mapAccentedChar(codepoint);
            i += seqLen;
        }

        return out;
    };

    auto resolveRadarOverlaps = [](int* xs, int* ys, const int* boxW, const int* topOffset, const int* bottomOffset, bool* dropped, int count, int screenMinX, int screenMaxX, int screenMinY, int screenMaxY) {
        const int passes = 4;
        for (int pass = 0; pass < passes; pass++) {
            for (int i = 1; i < count; i++) {
                for (int j = 0; j < i; j++) {
                    float dx = xs[i] - xs[j];

                    int topI = ys[i] + topOffset[i], bottomI = ys[i] + bottomOffset[i];
                    int topJ = ys[j] + topOffset[j], bottomJ = ys[j] + bottomOffset[j];
                    float centerI = (topI + bottomI) / 2.0f, centerJ = (topJ + bottomJ) / 2.0f;

                    if (fabs(dx) < 0.01 && fabs(centerI - centerJ) < 0.01) {

                        dx = (i % 2 == 0) ? 1.0f : -1.0f;
                    }

                    float overlapY = min(bottomI, bottomJ) - max(topI, topJ);
                    float overlapX = (boxW[i] + boxW[j]) / 2.0f - fabs(dx);

                    if (overlapX > 0 && overlapY > 0) {

                        if (overlapX < overlapY) {
                            xs[i] += (int)round((dx >= 0 ? 1.0f : -1.0f) * (overlapX + 1));
                        } else {
                            float dir = (centerI >= centerJ) ? 1.0f : -1.0f;
                            ys[i] += (int)round(dir * (overlapY + 1));
                        }
                    }
                }
            }
        }

        for (int i = 1; i < count; i++) {
            int left = xs[i] - boxW[i] / 2;
            int right = xs[i] + boxW[i] / 2;
            int top = ys[i] + topOffset[i];
            int bottom = ys[i] + bottomOffset[i];

            if (left < screenMinX || right > screenMaxX || top < screenMinY || bottom > screenMaxY) {
                dropped[i] = true;
                continue;
            }

            for (int j = 0; j < i; j++) {
                if (dropped[j]) continue;
                int topJ = ys[j] + topOffset[j], bottomJ = ys[j] + bottomOffset[j];
                bool yOverlap = min(bottom, bottomJ) - max(top, topJ) > 0;
                if (fabs(xs[i] - xs[j]) < (boxW[i] + boxW[j]) / 2.0f && yOverlap) {
                    dropped[i] = true;
                    break;
                }
            }
        }
    };

    if (config.flight_mode == "closest") {

        if (data.closest.callsign.length() == 0) {
            drawInfoScreen(icon_error, "No Aircraft");
            return;
        }

        const FlightAircraft& ac = data.closest;

        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextWrap(false);
        display.setFont();

        int16_t x1, y1; uint16_t w, h;

        String originCode = ac.has_route ? ac.origin_code : "N/A";
        String destCode = ac.has_route ? ac.destination_code : "N/A";

        display.setTextSize(2);
        display.setCursor(2, 1);
        display.print(originCode);

        display.getTextBounds(destCode.c_str(), 0, 0, &x1, &y1, &w, &h);
        display.setCursor(126 - w, 1);
        display.print(destCode);

        display.setTextSize(1);

        auto trimToHalfScreen = [&](String text) -> String {
            const int maxWidth = 64;
            display.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
            if ((int)w <= maxWidth) return text;

            while (text.length() > 0) {
                text = text.substring(0, text.length() - 1);
                display.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
                if ((int)w + 8 <= maxWidth) return text;
            }
            return "";
        };

        if (ac.has_route && data.origin_city.length() > 0) {
            String origin = sanitizeAscii(data.origin_city);
            String trimmed = trimToHalfScreen(origin);
            display.setCursor(2, 17);
            display.print(trimmed);
            if (trimmed != origin) {
                display.getTextBounds(trimmed.c_str(), 0, 0, &x1, &y1, &w, &h);
                display.drawBitmap(2 + w, 17, icon_dots, 8, 8, SSD1306_WHITE);
            }
        }
        if (ac.has_route && data.destination_city.length() > 0) {
            String dest = sanitizeAscii(data.destination_city);
            String trimmed = trimToHalfScreen(dest);
            display.getTextBounds(trimmed.c_str(), 0, 0, &x1, &y1, &w, &h);
            int startX = 126 - w - (trimmed != dest ? 8 : 0);
            display.setCursor(startX, 17);
            display.print(trimmed);
            if (trimmed != dest) {
                display.drawBitmap(startX + w, 17, icon_dots, 8, 8, SSD1306_WHITE);
            }
        }

        display.drawBitmap(58, 2, icon_plane, 12, 12, SSD1306_WHITE);

        auto drawRow = [&](int y, const unsigned char* icon, int iw, int ih, const String& leftVal, const String& rightVal, bool appendDegree = false) {
            display.drawBitmap(3, y, icon, iw, ih, SSD1306_WHITE);
            display.setCursor(11, y);
            display.print(leftVal);

            if (appendDegree) {
                display.getTextBounds(leftVal.c_str(), 0, 0, &x1, &y1, &w, &h);
                display.drawBitmap(11 + w + 1, y, degree_icon_small, 4, 4, SSD1306_WHITE);
            }

            String trimmedRight = trimToHalfScreen(rightVal);
            bool rightTrimmed = (trimmedRight != rightVal);
            display.getTextBounds(trimmedRight.c_str(), 0, 0, &x1, &y1, &w, &h);
            int rightX = 125 - w - (rightTrimmed ? 8 : 0);
            display.setCursor(rightX, y);
            display.print(trimmedRight);
            if (rightTrimmed) {
                display.drawBitmap(rightX + w, y, icon_dots, 8, 8, SSD1306_WHITE);
            }
        };

        String trackStr = isnan(ac.track_deg) ? "--" : String((int)round(ac.track_deg));
        float distanceVal = (config.flight_units == "metric") ? ac.distance_km : (ac.distance_km / kmPerNauticalMile);
        String distanceStr = (distanceVal < 10 ? String(distanceVal, 2) : String((int)round(distanceVal))) + (config.flight_units == "metric" ? " km" : " nm");

        String manufacturer = sanitizeAscii(data.aircraft_manufacturer);
        String aircraftType = sanitizeAscii(data.aircraft_type);
        String icao24 = ac.icao24;
        icao24.toUpperCase();
        String registration = ac.registration;
        registration.toUpperCase();
        String typeDesignator = ac.type_designator;
        typeDesignator.toUpperCase();
        String originCountry = ac.origin_country;
        originCountry.toUpperCase();
        String destCountry = ac.destination_country;
        destCountry.toUpperCase();
        String squawk = ac.squawk;

        String rightItems[8];
        int itemCount = 0;
        if (ac.has_route && originCountry.length() > 0 && destCountry.length() > 0) {
            rightItems[itemCount++] = originCountry + "-" + destCountry;
        }
        if (ac.callsign.length() > 0) rightItems[itemCount++] = ac.callsign;
        if (manufacturer.length() > 0) rightItems[itemCount++] = manufacturer;
        if (aircraftType.length() > 0) rightItems[itemCount++] = aircraftType;
        if (icao24.length() > 0) rightItems[itemCount++] = icao24;
        if (typeDesignator.length() > 0) rightItems[itemCount++] = typeDesignator;
        if (registration.length() > 0) rightItems[itemCount++] = registration;
        if (squawk.length() > 0) rightItems[itemCount++] = squawk;
        if (itemCount > 4) itemCount = 4;

        String rowRight[4] = {"", "", "", ""};
        int startRow = 4 - itemCount;
        for (int k = 0; k < itemCount; k++) rowRight[startRow + k] = rightItems[k];

        drawRow(29, icon_flight_e, 7, 5, formatVelocity(ac.velocity_kt), rowRight[0]);
        drawRow(38, icon_flight_n, 5, 7, formatAltitude(ac.altitude_ft), rowRight[1]);
        drawRow(47, icon_flight_ne, 5, 5, distanceStr, rowRight[2]);
        drawRow(56, icon_flight_sw, 5, 5, trackStr, rowRight[3], true);
        return;
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);

    int16_t x1, y1; uint16_t w, h;

    const int centerX = 64;
    const int radarTop = 4;
    const int radarBottom = 62;
    const int radarLeft = 3;
    const int radarRight = 124;
    const int centerY = (radarTop + radarBottom) / 2;

    display.setFont(&Picopixel);
    String radiusLabel = (config.flight_units == "metric")
        ? String((int)floor(config.flight_radius_nm * kmPerNauticalMile)) + "km"
        : String(config.flight_radius_nm) + "nm";
    display.getTextBounds(radiusLabel.c_str(), 0, 0, &x1, &y1, &w, &h);
    int labelHalfW = w / 2 + 3;

    display.drawFastHLine(0, 2, centerX - labelHalfW, SSD1306_WHITE);
    display.drawFastHLine(centerX + labelHalfW, 2, 128 - (centerX + labelHalfW), SSD1306_WHITE);
    display.fillTriangle(0, 2, 2, 0, 2, 4, SSD1306_WHITE);
    display.fillTriangle(127, 2, 125, 0, 125, 4, SSD1306_WHITE);
    display.drawFastVLine(centerX - labelHalfW, 1, 3, SSD1306_WHITE);
    display.drawFastVLine(centerX + labelHalfW, 1, 3, SSD1306_WHITE);
    display.setCursor(centerX - w / 2, -y1);
    display.print(radiusLabel);

    const int centerDotHalf = 2;
    display.fillCircle(centerX, centerY, 1, SSD1306_WHITE);

    if (data.aircraft_count == 0) {
        drawInfoScreen(icon_error, "No Aircraft");
        return;
    }

    auto textFor = [&](const String& info, const FlightAircraft& ac) -> String {
        if (info == "callsign") {
            return ac.callsign;
        } else if (info == "altitude") {
            return isnan(ac.altitude_ft) ? "--" : String((int)round(config.flight_units == "metric" ? ac.altitude_ft * 0.3048 : ac.altitude_ft)) + (config.flight_units == "metric" ? "m" : "ft");
        } else if (info == "velocity") {
            return isnan(ac.velocity_kt) ? "--" : String((int)round(config.flight_units == "metric" ? ac.velocity_kt * kmPerNauticalMile : ac.velocity_kt)) + (config.flight_units == "metric" ? "kmh" : "kt");
        } else if (info == "distance") {
            float distanceVal = (config.flight_units == "metric") ? ac.distance_km : (ac.distance_km / kmPerNauticalMile);
            return (distanceVal < 10 ? String(distanceVal, 1) : String((int)round(distanceVal))) + (config.flight_units == "metric" ? "km" : "nm");
        } else if (info == "route") {
            return ac.has_route ? (ac.origin_code + "-" + ac.destination_code) : "--";
        } else if (info == "track") {
            return isnan(ac.track_deg) ? "--" : String((int)round(ac.track_deg));
        } else if (info == "type") {
            return ac.type_designator.length() > 0 ? ac.type_designator : "--";
        }
        return "";
    };

    const int maxSlots = MAX_RADAR_AIRCRAFT + 1;
    int xs[maxSlots];
    int ys[maxSlots];
    int boxW[maxSlots];
    int topOffset[maxSlots];
    int bottomOffset[maxSlots];
    bool dropped[maxSlots] = {false};
    String primaries[maxSlots];
    String secondaries[maxSlots];
    bool secondaryVisible[maxSlots] = {false};

    xs[0] = centerX;
    ys[0] = centerY;
    boxW[0] = centerDotHalf * 2;
    topOffset[0] = -centerDotHalf;
    bottomOffset[0] = centerDotHalf;

    int slotCount = 1;

    float cosLat = cos(radians(config.latitude));
    if (fabs(cosLat) < 0.01) cosLat = 0.01;

    const float shortAxisKm = (config.flight_radius_nm * kmPerNauticalMile) / sqrt(5.0f);
    int verticalAvailablePx = min(centerY - radarTop, radarBottom - centerY) - 2;
    float pxPerKm = verticalAvailablePx / shortAxisKm;
    bool hasSecondary = (config.flight_secondary_info != "none");
    const int iconTopMargin = -4;

    for (int i = 0; i < data.aircraft_count; i++) {
        const FlightAircraft& ac = data.aircraft[i];
        int slot = slotCount++;

        float northKm = (ac.lat - config.latitude) * 111.32;
        float eastKm = (ac.lon - config.longitude) * 111.32 * cosLat;

        xs[slot] = centerX + (int)(eastKm * pxPerKm);
        ys[slot] = centerY - (int)(northKm * pxPerKm);

        String primaryRaw = textFor(config.flight_primary_info, ac);
        String secondaryRaw = hasSecondary ? textFor(config.flight_secondary_info, ac) : "";
        bool primaryIsPlaceholder = (primaryRaw == "--");
        bool secondaryIsPlaceholder = hasSecondary && (secondaryRaw == "--");

        String primary;
        String secondary;
        bool slotHasSecondary;
        if (!hasSecondary || (primaryIsPlaceholder != secondaryIsPlaceholder)) {
            primary = (hasSecondary && primaryIsPlaceholder) ? secondaryRaw : primaryRaw;
            slotHasSecondary = false;
        } else {
            primary = primaryRaw;
            secondary = secondaryRaw;
            slotHasSecondary = hasSecondary;
        }
        if (primary.length() > 7) primary = primary.substring(0, 7);

        primaries[slot] = primary;
        secondaries[slot] = secondary;
        secondaryVisible[slot] = slotHasSecondary;
        bool hasPrimary = primary.length() > 0;

        int textW = 0;
        int bottom = 4;

        if (hasPrimary) {
            display.getTextBounds(primaries[slot].c_str(), 0, 0, &x1, &y1, &w, &h);
            textW = w;
            bottom = 5 + (int)h;
        }

        if (slotHasSecondary) {
            display.getTextBounds(secondaries[slot].c_str(), 0, 0, &x1, &y1, &w, &h);
            textW = max((int)textW, (int)w);
            bottom = hasPrimary ? (bottom + 1 + (int)h) : (5 + (int)h);
        }

        boxW[slot] = textW + 2;
        topOffset[slot] = iconTopMargin;
        bottomOffset[slot] = bottom;
    }

    resolveRadarOverlaps(xs, ys, boxW, topOffset, bottomOffset, dropped, slotCount, radarLeft, radarRight, radarTop, radarBottom);

    for (int i = 0; i < data.aircraft_count; i++) {
        int slot = i + 1;
        if (dropped[slot]) continue;

        const FlightAircraft& ac = data.aircraft[i];
        int x = xs[slot];
        int y = ys[slot];

        if (isnan(ac.track_deg)) {
            display.drawRect(x - 1, y - 1, 3, 3, SSD1306_WHITE);
        } else {
            float deg = fmod(ac.track_deg, 360.0f);
            if (deg < 0) deg += 360.0f;

            const unsigned char* bmp; int bw, bh;
            if (deg >= 337.5 || deg < 22.5)  { bmp = icon_flight_n;  bw = 5; bh = 7; }
            else if (deg < 67.5)             { bmp = icon_flight_ne; bw = 5; bh = 5; }
            else if (deg < 112.5)            { bmp = icon_flight_e;  bw = 7; bh = 5; }
            else if (deg < 157.5)            { bmp = icon_flight_se; bw = 5; bh = 5; }
            else if (deg < 202.5)            { bmp = icon_flight_s;  bw = 5; bh = 7; }
            else if (deg < 247.5)            { bmp = icon_flight_sw; bw = 5; bh = 5; }
            else if (deg < 292.5)            { bmp = icon_flight_w;  bw = 7; bh = 5; }
            else                             { bmp = icon_flight_nw; bw = 5; bh = 5; }

            display.drawBitmap(x - bw / 2, y - bh / 2, bmp, bw, bh, SSD1306_WHITE);
        }

        bool hasPrimary = primaries[slot].length() > 0;
        int primaryTopY = y + 5;
        int primaryH = 0;

        if (hasPrimary) {
            display.getTextBounds(primaries[slot].c_str(), 0, 0, &x1, &y1, &w, &h);
            display.setCursor(x - (int)w / 2, primaryTopY - y1);
            display.print(primaries[slot]);
            primaryH = h;
        }

        if (!secondaryVisible[slot]) continue;

        display.getTextBounds(secondaries[slot].c_str(), 0, 0, &x1, &y1, &w, &h);
        int secondaryTopY = hasPrimary ? (primaryTopY + primaryH + 1) : primaryTopY;
        display.setCursor(x - (int)w / 2, secondaryTopY - y1);
        display.print(secondaries[slot]);
    }

    display.setFont();
}

bool isScreenEnabled(const AppState& state, int screenIndex) {
    const Config& config = state.config;

    switch (screenIndex) {
        case SCREEN_TIME:           return config.show_time;
        case SCREEN_CALENDAR:       return config.show_calendar;
        case SCREEN_WEATHER:        return config.show_weather;
        case SCREEN_AIR_QUALITY:    return config.show_aqi;
        case SCREEN_DAYLIGHT:       return config.show_daylight;
        case SCREEN_MOON:           return config.show_moon;
        case SCREEN_POPULATION:     return config.show_population;
        case SCREEN_FLIGHT: {
            if (!config.show_flight) return false;
            if (config.hide_empty_flight) {
                bool isInvalid = (config.flight_mode == "closest") ? (state.flight.closest.callsign.length() == 0) : (state.flight.aircraft_count == 0);
                if (isInvalid) return false;
            }
            return true;
        }
        case SCREEN_STOCK:          return config.show_stock;
        case SCREEN_CRYPTO:         return config.show_crypto;
        case SCREEN_CURRENCY:       return config.show_currency;
        case SCREEN_PC_MONITOR: {
            if (!config.show_pc) return false;
            if (config.hide_empty_pc) {
                bool isInvalid = (isnan(state.pc.cpu_percent) || state.pc.cpu_percent == 0) && (isnan(state.pc.mem_percent) || state.pc.mem_percent == 0);
                if (isInvalid) return false;
            }
            return true;
        }
        case SCREEN_PC_MEDIA: {
            if (!config.show_media) return false;
            if (config.hide_empty_media) {
                bool isInvalid = (state.media.status.length() == 0 || state.media.name.length() == 0 || state.media.author.length() == 0 || state.media.name.equalsIgnoreCase("Unknown"));
                if (isInvalid) return false;
            }
            return true;
        }
        case SCREEN_BAMBU: {
            if (!config.show_bambu) return false;
            if (config.hide_empty_bambu) {
                bool isInvalid = (state.bambu.status == "SYNCING");
                if (isInvalid) return false;
            }
            return true;
        }
        default: return false;
    }
}

void drawScreen(int screenIndex, const AppState& state, int subIndex = 0) {
    switch (screenIndex) {
        case SCREEN_TIME: drawTimeScreen(state.config, getCurrentTimeShort(state.config.time_format), getFullDate()); break;
        case SCREEN_CALENDAR: drawCalendarScreen(state.config, state.calendar); break;
        case SCREEN_WEATHER: drawWeatherScreen(state.config, state.weather, getCurrentTimeShort(state.config.time_format)); break;
        case SCREEN_AIR_QUALITY: drawAQIScreen(state.config, state.aqi, getCurrentTimeShort(state.config.time_format)); break;
        case SCREEN_DAYLIGHT: drawDaylightScreen(state.config, state.daylight); break;
        case SCREEN_MOON: drawMoonScreen(state.config, state.moon); break;
        case SCREEN_POPULATION: drawPopulationScreen(state.config, state.population); break;
        case SCREEN_FLIGHT: drawFlightScreen(state.config, state.flight); break;
        case SCREEN_STOCK: drawStockScreen(state.config, state.stocks[subIndex]); break;
        case SCREEN_CRYPTO: drawCryptoScreen(state.config, state.cryptos[subIndex]); break;
        case SCREEN_CURRENCY: drawCurrencyScreen(state.config, state.currencies[subIndex], state.config.currency_multipliers[subIndex]); break;
        case SCREEN_PC_MONITOR: drawPcScreen(state.pc); break;
        case SCREEN_PC_MEDIA: drawMediaScreen(state.media); break;
        case SCREEN_BAMBU: drawBambuScreen(state.bambu); break;
    }
}

void drawCurrentScreen(const AppState& state) {
    drawScreen(currentScreen, state, currentSubScreen);
}

int getFirstEnabledScreen(const AppState& state) {
    for (int i = 0; i < NUM_SCREENS; i++) {
        int screenId = state.config.screen_order[i];
        if (isScreenEnabled(state, screenId)) {
            return screenId;
        }
    }
    return state.config.screen_order[0];
}

void jumpToFirstEnabledScreen(const AppState& state) {
    currentScreen = getFirstEnabledScreen(state);
}

void setContrast(bool dim) {
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(dim ? CONTRAST_DIM : CONTRAST_MAX);
}

int getNextAnimationEffect(uint16_t mask) {
    int enabledAnims[10]; int count = 0;
    for (int i = 1; i <= 5; i++) { if (mask & (1 << i)) enabledAnims[count++] = i; }
    if (count == 0) return 0;
    return enabledAnims[random(0, count)];
}

void animateHorizontal(int prev, int pSub, int next, int nSub, const AppState& state) {
    display.clearDisplay(); drawScreen(prev, state, pSub); memcpy(screenBufferOld, display.getBuffer(), 1024);
    display.clearDisplay(); drawScreen(next, state, nSub); memcpy(screenBufferNew, display.getBuffer(), 1024);
    int step = 8;
    for (int offset = 0; offset <= 128; offset += step) {
        uint8_t* displayBuf = display.getBuffer();
        for (int page = 0; page < 8; page++) {
            int start = page * 128;
            if (offset < 128) memcpy(&displayBuf[start], &screenBufferOld[start + offset], 128 - offset);
            if (offset > 0) memcpy(&displayBuf[start + (128 - offset)], &screenBufferNew[start], offset);
        }
        display.display();
    }
}

void animateVertical(int prev, int pSub, int next, int nSub, const AppState& state) {
    display.clearDisplay(); drawScreen(prev, state, pSub); memcpy(screenBufferOld, display.getBuffer(), 1024);
    display.clearDisplay(); drawScreen(next, state, nSub); memcpy(screenBufferNew, display.getBuffer(), 1024);
    for (int step = 0; step <= 8; step++) {
        uint8_t* displayBuf = display.getBuffer();
        for (int page = 0; page < 8; page++) {
            int oldPageIdx = page + step; int newPageIdx = page - (8 - step); int destIndex = page * 128;
            if (oldPageIdx < 8) memcpy(&displayBuf[destIndex], &screenBufferOld[oldPageIdx * 128], 128);
            else if (newPageIdx >= 0) memcpy(&displayBuf[destIndex], &screenBufferNew[newPageIdx * 128], 128);
        }
        display.display(); delay(10);
    }
}

void animateDissolve(int prev, int pSub, int next, int nSub, const AppState& state) {
    display.clearDisplay(); drawScreen(prev, state, pSub); memcpy(screenBufferOld, display.getBuffer(), 1024);
    display.clearDisplay(); drawScreen(next, state, nSub); memcpy(screenBufferNew, display.getBuffer(), 1024);
    uint8_t* displayBuf = display.getBuffer();
    for (int step = 0; step < 8; step++) {
        uint8_t mask = 0;
        switch (step) {
            case 0: mask = 0b10000000; break; case 1: mask = 0b11000000; break; case 2: mask = 0b11100000; break; case 3: mask = 0b11100100; break;
            case 4: mask = 0b11110100; break; case 5: mask = 0b11111100; break; case 6: mask = 0b11111110; break; case 7: mask = 0b11111111; break;
        }
        for (int i = 0; i < 1024; i++) displayBuf[i] = (screenBufferNew[i] & mask) | (screenBufferOld[i] & ~mask);
        display.display(); delay(10);
    }
}

void animateCurtain(int prev, int pSub, int next, int nSub, const AppState& state) {
    display.clearDisplay(); drawScreen(prev, state, pSub); memcpy(screenBufferOld, display.getBuffer(), 1024);
    display.clearDisplay(); drawScreen(next, state, nSub); memcpy(screenBufferNew, display.getBuffer(), 1024);
    int maxRadius = 80; int step = 4; uint8_t* displayBuf = display.getBuffer();
    for (int r = 0; r <= maxRadius; r += step) {
        int startX = 64 - r; if (startX < 0) startX = 0;
        int endX = 64 + r; if (endX > 128) endX = 128;
        for (int x = 0; x < 128; x++) {
            bool insideCurtain = (x >= startX && x < endX);
            for (int page = 0; page < 8; page++) {
                int idx = x + (page * 128); displayBuf[idx] = insideCurtain ? screenBufferNew[idx] : screenBufferOld[idx];
            }
        }
        display.display();
    }
}

void animateBlinds(int prev, int pSub, int next, int nSub, const AppState& state) {
    display.clearDisplay(); drawScreen(prev, state, pSub); memcpy(screenBufferOld, display.getBuffer(), 1024);
    display.clearDisplay(); drawScreen(next, state, nSub); memcpy(screenBufferNew, display.getBuffer(), 1024);
    uint8_t* displayBuf = display.getBuffer();
    memcpy(displayBuf, screenBufferOld, 1024); display.display();
    int blindWidth = 16; int numBlinds = 8; int stepSize = 2;
    for (int progress = 0; progress < blindWidth; progress += stepSize) {
        for (int blind = 0; blind < numBlinds; blind++) {
            int blindStartX = blind * blindWidth;
            for (int i = 0; i < stepSize; i++) {
                int currentX = blindStartX + progress + i; if (currentX >= 128) continue;
                for (int page = 0; page < 8; page++) {
                    int idx = currentX + (page * 128); displayBuf[idx] = screenBufferNew[idx];
                }
            }
        }
        display.display(); delay(5);
    }
}

void animateTransition(int prevScreen, int prevSub, int nextScreen, int nextSub, const AppState& state) {
    int selectedEffect = getNextAnimationEffect(state.config.anim_mask);
    switch (selectedEffect) {
        case ANIM_SLIDE_HORIZONTAL: animateHorizontal(prevScreen, prevSub, nextScreen, nextSub, state); break;
        case ANIM_SLIDE_VERTICAL: animateVertical(prevScreen, prevSub, nextScreen, nextSub, state); break;
        case ANIM_DISSOLVE: animateDissolve(prevScreen, prevSub, nextScreen, nextSub, state); break;
        case ANIM_CURTAIN: animateCurtain(prevScreen, prevSub, nextScreen, nextSub, state); break;
        case ANIM_BLINDS: animateBlinds(prevScreen, prevSub, nextScreen, nextSub, state); break;
        default:
            display.clearDisplay(); drawScreen(nextScreen, state, nextSub); display.display(); break;
    }
}

void switchToNextScreen(const AppState& state) {
    const Config& config = state.config;

    if (currentScreen == SCREEN_STOCK && currentSubScreen + 1 < config.stock_count) {
        currentSubScreen++;
        animateTransition(currentScreen, currentSubScreen - 1, currentScreen, currentSubScreen, state);
        return;
    }
    if (currentScreen == SCREEN_CRYPTO && currentSubScreen + 1 < config.crypto_count) {
        currentSubScreen++;
        animateTransition(currentScreen, currentSubScreen - 1, currentScreen, currentSubScreen, state);
        return;
    }
    if (currentScreen == SCREEN_CURRENCY && currentSubScreen + 1 < config.currency_count) {
        currentSubScreen++;
        animateTransition(currentScreen, currentSubScreen - 1, currentScreen, currentSubScreen, state);
        return;
    }

    int oldScreen = currentScreen;
    int oldSubScreen = currentSubScreen;
    currentSubScreen = 0;

    int currentIndex = 0;
    for (int i = 0; i < NUM_SCREENS; i++) {
        if (config.screen_order[i] == currentScreen) {
            currentIndex = i; break;
        }
    }

    int checkIndex = currentIndex;
    int nextScreenCandidate = currentScreen;
    do {
        checkIndex++;
        if (checkIndex >= NUM_SCREENS) checkIndex = 0;
        int candidateId = config.screen_order[checkIndex];
        if (isScreenEnabled(state, candidateId)) {
            nextScreenCandidate = candidateId;
            break;
        }
    } while (checkIndex != currentIndex);

    if (oldScreen == nextScreenCandidate && oldSubScreen == 0) return;

    animateTransition(oldScreen, oldSubScreen, nextScreenCandidate, 0, state);
    currentScreen = nextScreenCandidate;
}

void switchToPreviousScreen(const AppState& state) {
    const Config& config = state.config;

    if (currentScreen == SCREEN_STOCK && currentSubScreen > 0) {
        currentSubScreen--;
        animateTransition(currentScreen, currentSubScreen + 1, currentScreen, currentSubScreen, state);
        return;
    }
    if (currentScreen == SCREEN_CRYPTO && currentSubScreen > 0) {
        currentSubScreen--;
        animateTransition(currentScreen, currentSubScreen + 1, currentScreen, currentSubScreen, state);
        return;
    }
    if (currentScreen == SCREEN_CURRENCY && currentSubScreen > 0) {
        currentSubScreen--;
        animateTransition(currentScreen, currentSubScreen + 1, currentScreen, currentSubScreen, state);
        return;
    }

    int oldScreen = currentScreen;
    int oldSubScreen = currentSubScreen;

    int currentIndex = 0;
    for (int i = 0; i < NUM_SCREENS; i++) {
        if (config.screen_order[i] == currentScreen) {
            currentIndex = i; break;
        }
    }

    int checkIndex = currentIndex;
    int prevScreenCandidate = currentScreen;
    do {
        checkIndex--;
        if (checkIndex < 0) checkIndex = NUM_SCREENS - 1;
        int candidateId = config.screen_order[checkIndex];
        if (isScreenEnabled(state, candidateId)) {
            prevScreenCandidate = candidateId;
            break;
        }
    } while (checkIndex != currentIndex);

    if (oldScreen == prevScreenCandidate && oldSubScreen == 0) return;

    animateTransition(oldScreen, oldSubScreen, prevScreenCandidate, 0, state);
    currentScreen = prevScreenCandidate;
    currentSubScreen = 0;
}

void handleScreenNavigation(bool goToPrevious) {
    if (goToPrevious) {
        Serial.println("Double Click: Switching to Previous Screen");
        switchToPreviousScreen(appState);
    } else {
        Serial.println("Single Click: Switching to Next Screen");
        switchToNextScreen(appState);
    }
    lastScreenSwitch = millis();
}

void handleSingleClick() {
    handleScreenNavigation(false);
}

void handleDoubleClick() {
    handleScreenNavigation(true);
}

void handleLongPress() {
    appState.config.screen_auto_cycle = !appState.config.screen_auto_cycle;

    if (appState.config.screen_auto_cycle) {
        Serial.println("Auto Cycle: ENABLED");
        drawInfoScreen(icon_unlock, "Auto Cycle On");
    } else {
        Serial.println("Auto Cycle: DISABLED (Screen Locked)");
        drawInfoScreen(icon_lock, "Auto Cycle Off");
    }
    display.display();
    delay(1000);

    display.clearDisplay();
    drawCurrentScreen(appState);
    display.display();

    lastScreenSwitch = millis();
}

void setup() {
    Serial.begin(115200);
    delay(100);

    struct tm seedTime = {};
    seedTime.tm_year = 2026 - 1900;
    seedTime.tm_mon = 8;
    seedTime.tm_mday = 11;
    seedTime.tm_hour = 12;
    time_t seedEpoch = mktime(&seedTime);
    struct timeval tv = { seedEpoch, 0 };
    settimeofday(&tv, nullptr);

    Wire.begin(appState.config.sda_pin, appState.config.scl_pin);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 allocation failed");
        for (;;);
    }

    if (appState.config.button_type == "switch") {
        button.setup(appState.config.button_pin, INPUT_PULLUP, true);
    } else {
        button.setup(appState.config.button_pin, INPUT, false);
    }
    button.attachClick(handleSingleClick);
    button.attachDoubleClick(handleDoubleClick);
    button.attachLongPressStart(handleLongPress);
    button.setDebounceTicks(50);
    button.setClickTicks(150);
    button.setPressTicks(500);
    button.reset();

    jumpToFirstEnabledScreen(appState);
    display.clearDisplay();
    drawCurrentScreen(appState);
    display.display();
    lastScreenSwitch = millis();
}

void loop() {
    button.tick();

    if (appState.config.screen_auto_cycle) {
        unsigned long intervalMs = (unsigned long)appState.config.screen_interval_sec * 1000;
        if (millis() - lastScreenSwitch >= intervalMs) {
            switchToNextScreen(appState);
            lastScreenSwitch = millis();
        }
    }
}
