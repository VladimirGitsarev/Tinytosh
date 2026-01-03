#include "DisplayService.h"
#include "images.h"
#include <Arduino.h>

String DisplayService::getWeatherDescription(int wmo_code) {
    if (wmo_code == 0) return "Clear Sky";
    if (wmo_code >= 1 && wmo_code <= 3) return "Cloudy";
    if (wmo_code >= 45 && wmo_code <= 48) return "Fog";
    if (wmo_code >= 51 && wmo_code <= 67) return "Rain";
    if (wmo_code >= 71 && wmo_code <= 77) return "Snow";
    if (wmo_code >= 95) return "Thunder";
    return "Unknown";
}

const unsigned char* DisplayService::getWeatherBitmap(int wmo_code) {
    if (wmo_code == 0) return icon_sun;
    else if (wmo_code >= 1 && wmo_code <= 3) return icon_cloud; 
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

void DisplayService::begin() {
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
    display.setTextSize(1);
    
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
    display.setTextColor(1);

    int16_t x1, y1;
    uint16_t w, h;

    if (config.date_display) {
        // Layout WITH Date
        display.setTextSize(3);
        display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor((128 - w) / 2, 10); 
        display.print(timeStr);

        display.setTextSize(1);
        display.getTextBounds(dateStr, 0, 0, &x1, &y1, &w, &h);
        display.setCursor((128 - w) / 2, 48); 
        display.print(dateStr);
    } else {
        // Layout WITHOUT Date
        display.setTextSize(4);
        display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);

        int xPos = (128 - w) / 2;
        int yPos = (64 - h) / 2 - y1; 
        
        display.setCursor(xPos, yPos);
        display.print(timeStr);
    }

    display.display();
}

void DisplayService::drawWeatherScreen(const Config& config, const WeatherData& data, const String& currentTime) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    int16_t x1, y1;
    uint16_t w, h;
    
    bool valid = !isnan(data.temp) && data.weather_code != -1;
    
    // 1. Header (City and Time)
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

    // 2. Main Temperature and Icon
    int yMiddleStart = ySeparator + 4; 
    int middleHeight = 35; 
    
    display.setTextSize(3); 
    
    String tempValueStr = valid ? 
                          (config.round_temps ? String((int)round(data.temp)) : String(data.temp, 1)) : 
                          "--";
    
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

    const unsigned char* iconBitmap = valid ? getWeatherBitmap(data.weather_code) : icon_cloud;
    display.drawBitmap(xIcon, yIcon, iconBitmap, iconSize, iconSize, SSD1306_WHITE); 

    // 3. Weather Description
    display.setTextSize(1);
    String desc = valid ? getWeatherDescription(data.weather_code) : "No Data";
    display.getTextBounds(desc.c_str(), 0, 0, &x1, &y1, &w, &h);
    
    int xDesc = xRightEdge - w; 
    int yDesc = yIcon + iconSize + 1; 
    display.setCursor(xDesc, yDesc);
    display.print(desc);
    
    // 4. Footer Stats (Feels Like, Humidity, Wind)
    int yFooter = 56; 
    int iconSmallSize = 8;
    int x1_start = 5;  
    int x2_start = 48; 
    int x3_start = 91; 

    // Feels Like
    String feelsLikeVal = valid ? 
                          (config.round_temps ? String((int)round(data.apparent_temperature)) : String(data.apparent_temperature, 1)) : 
                          "--";

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

    display.display();
}

void DisplayService::drawAQIScreen(const Config& config, const AirQualityData& data, const String& currentTime) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    int16_t x1, y1;
    uint16_t w, h;
    
    bool valid = (data.aqi != -1);
    
    // 1. Header (City and Time)
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

    // 2. Main AQI Value and Status Icon
    int yMiddleStart = ySeparator + 4; 
    int middleHeight = 35; 
    
    display.setTextSize(3); 
    String aqiStr = valid ? String(data.aqi) : "--";
    
    display.getTextBounds(aqiStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    int yAqi = yMiddleStart + ((middleHeight - h) / 2); 
    int xStartAqi = 5;

    display.setCursor(xStartAqi, yAqi); 
    display.print(aqiStr);

    display.setTextSize(1);
    int xLabel = xStartAqi + w + 4; // Position to the right of the big number

    // Print the Type (US or EU) slightly higher
    display.setCursor(xLabel + 2, yAqi + 4); 
    display.print(config.aqi_type); 

    // Print "AQI" directly below the type
    display.setCursor(xLabel, yAqi + 14); 
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
    int yFooter = 56; 
    int iconSmallSize = 8;
    int unitIconSize = 8;

    // Prepare values as strings without "km"
    String pm25Val = valid ? String((int)round(data.pm25)) : "--";
    String pm10Val = valid ? String((int)round(data.pm10)) : "--";
    String no2Val  = valid ? String((int)round(data.no2)) : "--";

    // --- LEFT ALIGN (PM 2.5) ---
    int x1_icon = 2;
    display.drawBitmap(x1_icon, yFooter, icon_small_particles, iconSmallSize, iconSmallSize, SSD1306_WHITE);
    int x1_text = x1_icon + iconSmallSize + 2;
    display.setCursor(x1_text, yFooter + 1);
    display.print(pm25Val);
    display.getTextBounds(pm25Val.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.drawBitmap(x1_text + w + 1, yFooter + 1, icon_ug, unitIconSize, unitIconSize, SSD1306_WHITE);

    // --- CENTER ALIGN (PM 10) ---
    display.getTextBounds(pm10Val.c_str(), 0, 0, &x1, &y1, &w, &h);
    // Total width = icon + spacing + text + spacing + unit icon
    int totalWidthCenter = iconSmallSize + 2 + w + 1 + unitIconSize;
    int x2_icon = (display.width() / 2) - (totalWidthCenter / 2);
    
    display.drawBitmap(x2_icon, yFooter, icon_big_particles, iconSmallSize, iconSmallSize, SSD1306_WHITE);
    int x2_text = x2_icon + iconSmallSize + 2;
    display.setCursor(x2_text, yFooter + 1);
    display.print(pm10Val);
    display.drawBitmap(x2_text + w + 1, yFooter + 1, icon_ug, unitIconSize, unitIconSize, SSD1306_WHITE);

    // --- RIGHT ALIGN (NO2) ---
    display.getTextBounds(no2Val.c_str(), 0, 0, &x1, &y1, &w, &h);
    int totalWidthRight = iconSmallSize + 2 + w + 1 + unitIconSize;
    int x3_icon = display.width() - totalWidthRight - 2;

    display.drawBitmap(x3_icon, yFooter, icon_no2, iconSmallSize, iconSmallSize, SSD1306_WHITE);
    int x3_text = x3_icon + iconSmallSize + 2;
    display.setCursor(x3_text, yFooter + 1);
    display.print(no2Val);
    display.drawBitmap(x3_text + w + 1, yFooter + 1, icon_ug, unitIconSize, unitIconSize, SSD1306_WHITE);

    display.display();
}

void DisplayService::drawPcScreen(const PcStats& pcStats) {
    // 1. Check if data is valid
    bool isInvalid = (isnan(pcStats.cpu_percent) || pcStats.cpu_percent == 0) &&
                     (isnan(pcStats.cpu_temp)    || pcStats.cpu_temp == 0)    &&
                     (isnan(pcStats.mem_percent) || pcStats.mem_percent == 0) &&
                     (isnan(pcStats.disk_percent)|| pcStats.disk_percent == 0);

    if (isInvalid) {
        drawNoData(); // Draw "No Data" scree if data invalid and exit method
        return; 
    }

    // 2. If data is valid, proceed with drawing
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(1);

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

    // CPU
    display.drawBitmap(0, 0, icon_cpu_percent, 16, 16, 1);
    drawInfilledBar(5, pcStats.cpu_percent);
    display.setCursor(TEXT_X, 4);
    display.print(String((int)round(pcStats.cpu_percent)) + "%");

    // Temp
    display.drawBitmap(0, 16, icon_cpu_temp, 16, 16, 1);
    drawInfilledBar(22, pcStats.cpu_temp);
    display.setCursor(TEXT_X, 21);
    display.print(String((int)round(pcStats.cpu_temp)) + "C");

    // RAM
    display.drawBitmap(0, 32, icon_ram_percent, 16, 16, 1);
    drawInfilledBar(37, pcStats.mem_percent);
    display.setCursor(TEXT_X, 36);
    display.print(String((int)round(pcStats.mem_percent)) + "%");

    // Disk
    display.drawBitmap(0, 48, icon_disk_percent, 16, 16, 1);
    drawInfilledBar(53, pcStats.disk_percent);
    display.setCursor(TEXT_X, 52);
    display.print(String((int)round(pcStats.disk_percent)) + "%");

    display.display();
}

void DisplayService::drawCryptoScreen(const CryptoData& data) {
    display.clearDisplay();
    display.setTextColor(1);
    display.setTextWrap(false);

    // 1. Symbol
    display.setTextSize(3);
    display.setCursor(4, 6);
    display.print(data.symbol);

    // 2. Price
    display.setTextSize(2);
    display.setCursor(4, 44);
    display.print("$" + String(data.price_usd));

    // 3. Arrow
    bool isPositive = (data.percent_change_24h >= 0);
    const unsigned char* arrowIcon = isPositive ? icon_arrow_up : icon_arrow_down;
    display.drawBitmap(102, 3, arrowIcon, 15, 15, 1);

    // 4. Percentage Change
    display.setTextSize(1);
    display.setCursor(95, 22);
    
    // Add '+' sign for positive changes
    String trendPrefix = isPositive ? "+" : ""; 
    display.print(trendPrefix + String(data.percent_change_24h, 1) + "%");

    display.display();
}

void DisplayService::drawNoData() {
    display.clearDisplay();

    display.drawRect(1, 1, 126, 62, 1);
    display.drawRect(3, 3, 122, 58, 1);

    display.setTextColor(1);
    display.setTextSize(2);
    display.setTextWrap(false);
    display.setCursor(23, 25);
    display.print("No data");

    display.display();
}

bool DisplayService::isScreenEnabled(const Config& config, int screenIndex) {
    switch (screenIndex) {
        case SCREEN_TIME:           return config.show_time;
        case SCREEN_WEATHER:        return config.show_weather;
        case SCREEN_AIR_QUALITY:    return config.show_aqi;
        case SCREEN_PC_MONITOR:     return config.show_pc;
        case SCREEN_CRYPTO:         return config.show_crypto;
        default:                    return false;
    }
}