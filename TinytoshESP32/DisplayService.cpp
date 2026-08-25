#include "DisplayService.h"
#include "DaylightService.h"
#include "WeatherService.h"
#include "images.h"
#include <Arduino.h>
#include <Fonts/Picopixel.h>
#include <Fonts/Org_01.h>

const unsigned char* DisplayService::getWeatherBitmap(int wmo_code, bool is_day) {
    if (wmo_code == 0) {
        if (is_day) {
            return icon_sun;
        }
        return icon_moon;
    }
    else if (wmo_code >= 1 && wmo_code <= 3) {
        if (is_day) {
            return icon_cloud;
        }
        return icon_cloud_moon; 
    }
    else if (wmo_code >= 45 && wmo_code <= 48) return icon_fog;
    else if (wmo_code >= 51 && wmo_code <= 67) return icon_rain;
    else if (wmo_code >= 71 && wmo_code <= 77) return icon_snow;
    else if (wmo_code >= 95) return icon_thunder;
    return icon_cloud;
}

const unsigned char* DisplayService::getAQIBitmap(int val, bool is_eu) {
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

DisplayService::DisplayService(int width, int height, int reset_pin) : 
    display(width, height, &Wire, reset_pin) {}

void DisplayService::begin(int sda, int scl) {
    Wire.begin(sda, scl);
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        Serial.println(F("DisplayService: SSD1306 allocation failed."));
    } else {
        Serial.println("DisplayService: Display initialized.");
        display.clearDisplay();
        display.drawBitmap(0, 0, icon_hello, 128, 64, SSD1306_WHITE);
        display.display();
    }
}

void DisplayService::showOLEDStatus(std::initializer_list<String> lines, bool clear) {
    if (clear) display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setFont(); 
    
    int lineHeight = 10;
    int cursorY = 0;
    int16_t x1, y1;
    uint16_t w, h;
    
    for (const String& line : lines) {
        if (line == "\n") { 
            cursorY += lineHeight / 2;
            continue;
        }

        display.getTextBounds(line.c_str(), 0, 0, &x1, &y1, &w, &h);
        int xStart = (display.width() - w) / 2;
        
        display.setCursor(xStart, cursorY);
        display.print(line);
        
        cursorY += lineHeight;
        
        if (cursorY >= display.height()) break;
    }
    
    display.display();
}

void DisplayService::drawTimeScreen(const Config& config, String timeStr, String dateStr) {
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

void DisplayService::drawCalendarScreen(const Config& config, const CalendarData& calendar) {
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

    // 1. Check if Today is a Holiday
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

    // 2. Draw Main Text
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

    // 3. Stop if in Minimalistic Mode
    if (config.calendar_minimal) return;

    // 4. Right Column Grid Setup
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

void DisplayService::drawWeatherScreen(const Config& config, const WeatherData& data, const String& currentTime) {
    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setFont(); 
    
    int16_t x1, y1;
    uint16_t w, h;
    
    bool valid = !isnan(data.temp) && data.weather_code != -1;
    
    bool hideBar = config.weather_hide_bar;

    // 1. Header (City and Time)
    if (!hideBar) {
        display.setTextSize(1);
        String cityStr = valid ? config.city : "No Location";
        
        int yHeader = 2; 
        display.setCursor(2, yHeader);
        display.print(cityStr);

        display.getTextBounds(currentTime.c_str(), 0, 0, &x1, &y1, &w, &h);
        int xTime = display.width() - w - 2;
        display.setCursor(xTime, yHeader);
        display.print(currentTime);

        int ySeparator = 14; 
        display.drawFastHLine(0, ySeparator, display.width(), SSD1306_WHITE);
    }

    // 2. Main Temperature and Icon
    int yMiddleStart = hideBar ? 7 : 18; 
    int middleHeight = 35;
    
    display.setTextSize(3); 
    
    String tempValueStr = valid ? (config.round_temps ? String((int)round(data.temp)) : String(data.temp, 1)) : "--";
    
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

    // 3. Weather Description
    display.setTextSize(1);
    String desc = valid ? WeatherService::getWeatherDescription(data.weather_code) : "No Data";
    display.getTextBounds(desc.c_str(), 0, 0, &x1, &y1, &w, &h);
    
    int xDesc = xRightEdge - w; 
    int yDesc = yIcon + iconSize + 1; 
    display.setCursor(xDesc, yDesc);
    display.print(desc);
    
    // 4. Footer Stats (Feels Like, Humidity, Wind)
    int yFooter = hideBar ? 49 : 56; 
    int iconSmallSize = 8;
    int x1_start = 5;  
    int x2_start = 48; 
    int x3_start = 91; 

    String feelsLikeVal = valid ? (config.round_temps ? String((int)round(data.apparent_temperature)) : String(data.apparent_temperature, 1)) : "--";

    display.drawBitmap(x1_start, yFooter, icon_feel, iconSmallSize, iconSmallSize, SSD1306_WHITE); 
    int x1_value = x1_start + iconSmallSize + 2; 
    display.getTextBounds(feelsLikeVal.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(x1_value, yFooter + 1);
    display.print(feelsLikeVal);
    int x1_deg = x1_value + w;
    display.drawBitmap(x1_deg, yFooter + 1, degree_icon_small, 4, 4, SSD1306_WHITE);

    // Humidity
    String humVal = valid ? String(data.humidity) + "%" : "--%";
    display.drawBitmap(x2_start, yFooter, icon_drop, iconSmallSize, iconSmallSize, SSD1306_WHITE);
    int x2_value = x2_start + iconSmallSize + 2;
    display.setCursor(x2_value, yFooter + 1);
    display.print(humVal);

    // Wind
    String windVal = valid ? String((int)round(data.wind_speed)) + "km" : "--km";
    display.drawBitmap(x3_start, yFooter, icon_wind, iconSmallSize, iconSmallSize, SSD1306_WHITE); 
    int x3_value = x3_start + iconSmallSize + 2;
    display.setCursor(x3_value, yFooter + 1);
    display.print(windVal);
}

void DisplayService::drawAQIScreen(const Config& config, const AirQualityData& data, const String& currentTime) {
    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setFont(); 
    
    int16_t x1, y1;
    uint16_t w, h;
    
    bool valid = (data.aqi != -1);
    
    bool hideBar = config.aqi_hide_bar;

    // 1. Header (City and Time)
    if (!hideBar) {
        display.setTextSize(1);
        String cityStr = valid ? config.city : "No Location";
        
        int yHeader = 2; 
        display.setCursor(2, yHeader);
        display.print(cityStr);

        display.getTextBounds(currentTime.c_str(), 0, 0, &x1, &y1, &w, &h);
        int xTime = display.width() - w - 2;
        display.setCursor(xTime, yHeader);
        display.print(currentTime);

        int ySeparator = 14; 
        display.drawFastHLine(0, ySeparator, display.width(), SSD1306_WHITE);
    }

    // 2. Main AQI Value and Status Icon
    int yMiddleStart = hideBar ? 7 : 18; 
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

    // Print the Type (US or EU) slightly higher
    display.setCursor(xLabel, yAqi); 
    display.print(config.aqi_type); 

    // Print "AQI" directly below the type
    display.setCursor(xLabel, yAqi + 9); 
    display.print("AQI");

    int iconSize = 24; 
    int rightMargin = 5;
    int xRightEdge = display.width() - rightMargin; 
    int xIcon = xRightEdge - iconSize; 
    int yIcon = yMiddleStart + 1; 

    const unsigned char* aqiIcon = valid ? getAQIBitmap(data.aqi, config.aqi_type == "EU") : icon_neutral;
    display.drawBitmap(xIcon, yIcon, aqiIcon, iconSize, iconSize, SSD1306_WHITE); 

    // 3. Status Description
    display.setTextSize(1);
    String desc = valid ? data.status : "No Data";
    display.getTextBounds(desc.c_str(), 0, 0, &x1, &y1, &w, &h);
    
    int xDesc = xRightEdge - w; 
    int yDesc = yIcon + iconSize + 1; 
    display.setCursor(xDesc, yDesc);
    display.print(desc);
    
    // 4. Footer Stats (PM2.5, PM10, NO2) - Calculated Alignment
    int yFooter = hideBar ? 49 : 56; 
    int iconSmallSize = 8;
    int unitIconSize = 8;

    String pm25Val = valid ? String((int)round(data.pm25)) : "--";
    String pm10Val = valid ? String((int)round(data.pm10)) : "--";
    String no2Val  = valid ? String((int)round(data.no2)) : "--";

    int x1_icon = 2;
    display.drawBitmap(x1_icon, yFooter, icon_small_particles, iconSmallSize, iconSmallSize, SSD1306_WHITE);
    int x1_text = x1_icon + iconSmallSize + 2;
    display.setCursor(x1_text, yFooter + 1);
    display.print(pm25Val);
    display.getTextBounds(pm25Val.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.drawBitmap(x1_text + w + 1, yFooter + 1, icon_ug, unitIconSize, unitIconSize, SSD1306_WHITE);

    display.getTextBounds(pm10Val.c_str(), 0, 0, &x1, &y1, &w, &h);
    int totalWidthCenter = iconSmallSize + 2 + w + 1 + unitIconSize;
    int x2_icon = (display.width() / 2) - (totalWidthCenter / 2);
    
    display.drawBitmap(x2_icon, yFooter, icon_big_particles, iconSmallSize, iconSmallSize, SSD1306_WHITE);
    int x2_text = x2_icon + iconSmallSize + 2;
    display.setCursor(x2_text, yFooter + 1);
    display.print(pm10Val);
    display.drawBitmap(x2_text + w + 1, yFooter + 1, icon_ug, unitIconSize, unitIconSize, SSD1306_WHITE);

    display.getTextBounds(no2Val.c_str(), 0, 0, &x1, &y1, &w, &h);
    int totalWidthRight = iconSmallSize + 2 + w + 1 + unitIconSize;
    int x3_icon = display.width() - totalWidthRight - 2;

    display.drawBitmap(x3_icon, yFooter, icon_gas, iconSmallSize, iconSmallSize, SSD1306_WHITE);
    int x3_text = x3_icon + iconSmallSize + 2;
    display.setCursor(x3_text, yFooter + 1);
    display.print(no2Val);
    display.drawBitmap(x3_text + w + 1, yFooter + 1, icon_ug, unitIconSize, unitIconSize, SSD1306_WHITE);
}

void DisplayService::drawDaylightScreen(const Config& config, const DaylightData& data) {
    if (data.sunrise_mins == -1) {
        drawInfoScreen(nullptr, "No Daylight Data"); 
        return; 
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);

    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    int currentMins = timeinfo.tm_hour * 60 + timeinfo.tm_min;

    // 1. Calculate Day/Night state and progress
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

    // 2. Assign Times and Icons
    int leftMins = isDay ? data.sunrise_mins : data.sunset_mins;
    int rightMins = isDay ? data.sunset_mins : data.sunrise_mins;
    int centerMins = isDay ? data.noon_mins : (data.noon_mins + 720) % 1440; 
    int displayLengthMins = isDay ? data.length_mins : (1440 - data.length_mins); 
    
    const unsigned char* leftIcon = isDay ? icon_sun_rise : icon_sun_set;
    const unsigned char* rightIcon = isDay ? icon_sun_set : icon_sun_rise;
    const unsigned char* centerIcon = isDay ? icon_sun : icon_moon;

    // 3. Layout Positioning 
    int iconY = config.daylight_minimal ? 14 : 2; 
    int textY = iconY + 24 + 4;

    int16_t x1, y1; uint16_t w, h;

    // 4. Main Icons
    display.drawBitmap(19 - 12, iconY, leftIcon, 24, 24, 1);
    display.drawBitmap(64 - 12, iconY, centerIcon, 24, 24, 1);
    display.drawBitmap(110 - 12, iconY, rightIcon, 24, 24, 1);

    // 5. Main Times
    display.setFont(); 
    display.setTextSize(1);
    
    String leftStr = TimeService::formatMinsFromMidnight(leftMins, config.time_format, false);
    display.getTextBounds(leftStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(19 - (w / 2), textY);
    display.print(leftStr);

    String centerStr = TimeService::formatMinsFromMidnight(centerMins, config.time_format, false);
    display.getTextBounds(centerStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(64 - (w / 2), textY);
    display.print(centerStr);

    String rightStr = TimeService::formatMinsFromMidnight(rightMins, config.time_format, false);
    display.getTextBounds(rightStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(110 - (w / 2), textY);
    display.print(rightStr);

    if (config.daylight_minimal) {
        return;
    }

    // 6. Progress Bar
    display.drawRect(2, 45, 124, 6, 1);
    int fillW = (int)((constrain(progress, 0, 100) / 100.0) * 120);
    if (fillW > 0) display.fillRect(4, 47, fillW, 2, 1);

    // 7. Progress Percentage
    String progStr = String(progress) + "%";
    display.getTextBounds(progStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(64 - (w / 2), 55);
    display.print(progStr);

    // 8. Secondary Bottom Text
    String lengthStr = TimeService::formatDurationMins(displayLengthMins);
    lengthStr.replace(" ", "");
    display.setCursor(2, 55);
    display.print(lengthStr);

    String timeLeftStr = String(mins_left / 60) + "h" + String(mins_left % 60) + "m";
    display.getTextBounds(timeLeftStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(126 - w, 55);
    display.print(timeLeftStr);
}

void DisplayService::drawMoonScreen(const Config& config, const MoonData& data) {
    if (data.fracillum == -1) {
        drawInfoScreen(nullptr, "No Moon Data");
        return;
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setFont();

    int16_t x1, y1; uint16_t w, h;
    bool isMinimal = config.moon_minimal;

    // 1. Phase Name
    String phaseStr = data.curphase;
    phaseStr.toUpperCase();
    display.getTextBounds(phaseStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, 55);
    display.print(phaseStr);

    int cx = 64;
    int cy = 31;
    int r = 17;

    if (!isMinimal) {
        // 2. Illumination
        String illumStr = String(data.fracillum) + "%";
        display.getTextBounds(illumStr.c_str(), 0, 0, &x1, &y1, &w, &h);
        display.setCursor((128 - w) / 2, 2);
        display.print(illumStr);

        // 3. Moonrise
        String riseStr = (data.rise_mins != -1) ? TimeService::formatMinsFromMidnight(data.rise_mins, config.time_format) : "--:--";
        display.setCursor(6, 29);
        display.print(riseStr);
        display.drawBitmap(17, 21, icon_up, 7, 4, 1);

        // 4. Moonset
        String setStr = (data.set_mins != -1) ? TimeService::formatMinsFromMidnight(data.set_mins, config.time_format) : "--:--";
        display.setCursor(92, 29);
        display.print(setStr);
        display.drawBitmap(103, 21, icon_down, 7, 4, 1);
    } else {
        cy = 27; 
        r = 22;  
    }

    // 5. Moon
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

void DisplayService::drawCryptoScreen(const Config& config, const CryptoData& data) {
    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setFont(); 

    // 1. Symbol
    display.setTextSize(3);
    display.setCursor(4, 6);
    display.print(data.symbol);

    // 2. Full Name
    if (config.crypto_fn) {
        display.setTextSize(1);
        display.setCursor(4, 32); 
        String displayName = data.name;
        int maxLen = 20; 
        if (displayName.length() > maxLen) displayName = displayName.substring(0, maxLen - 3) + "...";
        displayName.toUpperCase();
        display.print(displayName);
    }

    // 3. Price
    display.setTextSize(2);
    display.setCursor(4, 44);
    display.print("$" + String(data.price_usd));

    // 4. Arrow & Percentage
    bool isPositive = (data.percent_change_24h >= 0);
    const unsigned char* arrowIcon = isPositive ? icon_arrow_up : icon_arrow_down;
    display.drawBitmap(102, 3, arrowIcon, 15, 15, 1);

    display.setTextSize(1);
    display.setCursor(95, 22);
    String trendPrefix = isPositive ? "+" : ""; 
    display.print(trendPrefix + String(data.percent_change_24h, 1) + "%");
}

void DisplayService::drawCurrencyScreen(const Config& config, const CurrencyData& data, int multiplier) {
    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setFont(); 

    // 1. Base Currency Symbol
    display.setTextSize(3);
    display.setCursor(4, 6);
    display.print(data.base);

    // 2. Full Currency Name
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

    // 3. Calculate Rate & Decimals using the passed Multiplier
    float displayRate = data.rate * multiplier;
    int decimals = (displayRate < 10.0) ? 3 : (displayRate < 100.0) ? 2 : (displayRate < 1000.0) ? 1 : 0;

    // 4. Rate and Target Currency
    display.setTextSize(2);
    display.setCursor(4, 44);
    display.print(String(displayRate, decimals) + " " + data.target);

    // 5. Context helper & Equals sign
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

void DisplayService::drawStockScreen(const Config& config, const StockData& data) {
    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setFont(); 

    // 1. Symbol
    display.setTextSize(3);
    display.setCursor(4, 6);
    display.print(data.symbol);

    // 2. Company Name
    if (config.stock_fn) {
        display.setTextSize(1);
        display.setCursor(4, 32);
        String displayName = data.name;
        int maxLen = 20; 
        if (displayName.length() > maxLen) displayName = displayName.substring(0, maxLen - 3) + "...";
        displayName.toUpperCase();
        display.print(displayName);
    }

    // 3. Price
    display.setTextSize(2);
    display.setCursor(4, 44);
    display.print("$" + String(data.price));

    // 4. Arrow & Percentage
    bool isPositive = (data.percent_change >= 0);
    const unsigned char* arrowIcon = isPositive ? icon_arrow_up : icon_arrow_down;
    display.drawBitmap(102, 3, arrowIcon, 15, 15, 1);

    display.setTextSize(1);
    display.setCursor(95, 22);
    String trendPrefix = isPositive ? "+" : ""; 
    display.print(trendPrefix + String(data.percent_change, 1) + "%");
}

void DisplayService::drawPcScreen(const PcStats& pcStats) {
    bool isInvalid = (isnan(pcStats.cpu_percent) || pcStats.cpu_percent == 0) && (isnan(pcStats.mem_percent) || pcStats.mem_percent == 0); 

    if (isInvalid) {
        drawInfoScreen(icon_monitor); 
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

    // 1. CPU
    display.drawBitmap(0, 0, icon_cpu_percent, 16, 16, 1);
    drawInfilledBar(5, pcStats.cpu_percent);
    display.setCursor(TEXT_X, 4);
    display.print(String((int)round(pcStats.cpu_percent)) + "%");

    // 2. RAM
    display.drawBitmap(0, 16, icon_ram_percent, 16, 16, 1);
    drawInfilledBar(21, pcStats.mem_percent);
    display.setCursor(TEXT_X, 20);
    display.print(String((int)round(pcStats.mem_percent)) + "%");

    // 3. Disk
    display.drawBitmap(0, 32, icon_disk_percent, 16, 16, 1);
    drawInfilledBar(37, pcStats.disk_percent);
    display.setCursor(TEXT_X, 36);
    display.print(String((int)round(pcStats.disk_percent)) + "%");

    // 4. Download
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

void DisplayService::drawMediaScreen(const PcMedia& media) {
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

void DisplayService::drawBambuScreen(const BambuData& data) {
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
        // 1. Printer Icon & IDLE Text (Left side)
        display.drawBitmap(22, 9, icon_printer, 32, 32, 1);
        
        display.setTextSize(1);
        display.setCursor(26, 45);
        display.print("IDLE");

        display.setTextSize(2);

        // 2. Nozzle Temp (Top Right)
        String n_val = String((int)round(data.nozzle_temp));
        int n_circle_x = 79 + (n_val.length() * 12) + 5; 
        
        display.drawBitmap(65, 17, icon_nozzle, 8, 8, 1);
        display.setCursor(79, 13);
        display.print(n_val);
        display.drawCircle(n_circle_x, 16, 3, SSD1306_WHITE); 

        // 3. Bed Temp (Bottom Right)
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

    // 1. Icons
    display.drawBitmap(4, 4, icon_nozzle, 8, 8, 1);
    display.drawBitmap(4, 16, icon_bed, 8, 8, 1);
    display.drawBitmap(116, 4, icon_part_fan, 8, 8, 1);
    display.drawBitmap(116, 16, icon_aux_fan, 8, 8, 1);

    // 2. Temperatures (Smart Slash Alignment)
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

    // 3. Fans (Right Alignment)
    String p_fan = String(data.fan_part) + "%";
    String a_fan = String(data.fan_aux) + "%";

    display.getTextBounds(p_fan.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(116 - 4 - w, 4);
    display.print(p_fan);

    display.getTextBounds(a_fan.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor(116 - 4 - w, 16);
    display.print(a_fan);

    // 4. File Name (Center Alignment & Truncation)
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

    // 5. Progress Bar
    display.drawRect(4, 43, 120, 6, 1);
    int fillW = (int)((constrain(data.progress, 0, 100) / 100.0) * 116);
    if (fillW > 0) display.fillRect(6, 45, fillW, 2, 1);

    // 6. Stats
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

void DisplayService::drawInfoScreen(const unsigned char* image, String text) {
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

bool DisplayService::isScreenEnabled(const AppState& state, int screenIndex) {
    const Config& config = state.config;

    switch (screenIndex) {
        case SCREEN_TIME:           return config.show_time;
        case SCREEN_CALENDAR:       return config.show_calendar;
        case SCREEN_WEATHER:        return config.show_weather;
        case SCREEN_AIR_QUALITY:    return config.show_aqi;
        case SCREEN_DAYLIGHT:       return config.show_daylight;
        case SCREEN_MOON:           return config.show_moon;
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

void DisplayService::drawScreen(int screenIndex, const AppState& state, int subIndex) {
  switch(screenIndex) {
    case SCREEN_TIME: drawTimeScreen(state.config, TimeService::getCurrentTimeShort(state.config.time_format), TimeService::getFullDate()); break;
    case SCREEN_CALENDAR: drawCalendarScreen(state.config, state.calendar); break;
    case SCREEN_WEATHER: drawWeatherScreen(state.config, state.weather, TimeService::getCurrentTimeShort(state.config.time_format)); break;
    case SCREEN_AIR_QUALITY: drawAQIScreen(state.config, state.aqi, TimeService::getCurrentTimeShort(state.config.time_format)); break;
    case SCREEN_DAYLIGHT: drawDaylightScreen(state.config, state.daylight); break;
    case SCREEN_MOON: drawMoonScreen(state.config, state.moon); break;
    case SCREEN_STOCK: drawStockScreen(state.config, state.stocks[subIndex]); break;
    case SCREEN_CRYPTO: drawCryptoScreen(state.config, state.cryptos[subIndex]); break;
    case SCREEN_CURRENCY: drawCurrencyScreen(state.config, state.currencies[subIndex], state.config.currency_multipliers[subIndex]); break;
    case SCREEN_PC_MONITOR: drawPcScreen(state.pc); break;
    case SCREEN_PC_MEDIA: drawMediaScreen(state.media); break;
    case SCREEN_BAMBU: drawBambuScreen(state.bambu); break;
  }
}

int DisplayService::getNextAnimationEffect(uint16_t mask) {
    int enabledAnims[10]; int count = 0;
    for (int i = 1; i <= 5; i++) { if (mask & (1 << i)) enabledAnims[count++] = i; }
    if (count == 0) return 0;
    return enabledAnims[random(0, count)];
}

void DisplayService::animateTransition(int prevScreen, int prevSub, int nextScreen, int nextSub, const AppState& state) {
    int selectedEffect = getNextAnimationEffect(state.config.anim_mask);
    switch(selectedEffect) {
        case ANIM_SLIDE_HORIZONTAL: animateHorizontal(prevScreen, prevSub, nextScreen, nextSub, state); break;
        case ANIM_SLIDE_VERTICAL: animateVertical(prevScreen, prevSub, nextScreen, nextSub, state); break;
        case ANIM_DISSOLVE: animateDissolve(prevScreen, prevSub, nextScreen, nextSub, state); break;
        case ANIM_CURTAIN: animateCurtain(prevScreen, prevSub, nextScreen, nextSub, state); break;
        case ANIM_BLINDS: animateBlinds(prevScreen, prevSub, nextScreen, nextSub, state); break;
        default:
            display.clearDisplay(); drawScreen(nextScreen, state, nextSub); display.display(); break;
    }
}

void DisplayService::animateHorizontal(int prev, int pSub, int next, int nSub, const AppState& state) {
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

void DisplayService::animateVertical(int prev, int pSub, int next, int nSub, const AppState& state) {
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

void DisplayService::animateDissolve(int prev, int pSub, int next, int nSub, const AppState& state) {
  display.clearDisplay(); drawScreen(prev, state, pSub); memcpy(screenBufferOld, display.getBuffer(), 1024);
  display.clearDisplay(); drawScreen(next, state, nSub); memcpy(screenBufferNew, display.getBuffer(), 1024);
  uint8_t* displayBuf = display.getBuffer();
  for (int step = 0; step < 8; step++) {
    uint8_t mask = 0;
    switch(step) {
        case 0: mask = 0b10000000; break; case 1: mask = 0b11000000; break; case 2: mask = 0b11100000; break; case 3: mask = 0b11100100; break;
        case 4: mask = 0b11110100; break; case 5: mask = 0b11111100; break; case 6: mask = 0b11111110; break; case 7: mask = 0b11111111; break;
    }
    for (int i = 0; i < 1024; i++) displayBuf[i] = (screenBufferNew[i] & mask) | (screenBufferOld[i] & ~mask);
    display.display(); delay(10);
  }
}

void DisplayService::animateCurtain(int prev, int pSub, int next, int nSub, const AppState& state) {
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

void DisplayService::animateBlinds(int prev, int pSub, int next, int nSub, const AppState& state) {
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