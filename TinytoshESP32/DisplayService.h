#ifndef DISPLAY_SERVICE_H
#define DISPLAY_SERVICE_H

#include "structs.h"
#include <Adafruit_SSD1306.h>
#include <Wire.h>

class DisplayService {
public:
    DisplayService(int width, int height, int reset_pin);
    void begin();
    void showOLEDStatus(std::initializer_list<String> lines, bool clear);
    void drawTimeScreen(const Config& config, String timeStr, String dateStr);
    void drawWeatherScreen(const Config& config, const WeatherData& data, const String& currentTime);
    void drawAQIScreen(const Config& config, const AirQualityData& data, const String& currentTime);
    void drawPcScreen(const PcStats& pcStats);
    void drawCryptoScreen(const CryptoData& data);
    void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
    void drawNoData();
    bool isScreenEnabled(const Config& config, int screenIndex);

private:
    Adafruit_SSD1306 display;
    const int lineHeight = 10;
    String getWeatherDescription(int wmo_code);
    const unsigned char* getWeatherBitmap(int wmo_code);
    const unsigned char* getAQIBitmap(int aqi, bool is_eu);
};

#endif