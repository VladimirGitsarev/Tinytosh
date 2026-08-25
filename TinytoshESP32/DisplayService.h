#ifndef DISPLAY_SERVICE_H
#define DISPLAY_SERVICE_H

#include "structs.h"
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "TimeService.h"

class DisplayService {
public:
    Adafruit_SSD1306 display;

    DisplayService(int width, int height, int reset_pin);
    void begin(int sda, int scl);
    
    void showOLEDStatus(std::initializer_list<String> lines, bool clear = true);
    void drawTimeScreen(const Config& config, String timeStr, String dateStr);
    void drawCalendarScreen(const Config& config, const CalendarData& calendar);
    void drawWeatherScreen(const Config& config, const WeatherData& data, const String& currentTime);
    void drawAQIScreen(const Config& config, const AirQualityData& data, const String& currentTime);
    void drawDaylightScreen(const Config& config, const DaylightData& data);
    void drawMoonScreen(const Config& config, const MoonData& data);
    void drawCryptoScreen(const Config& config, const CryptoData& data);
    void drawCurrencyScreen(const Config& config, const CurrencyData& data, int multiplier);
    void drawStockScreen(const Config& config, const StockData& data);
    void drawPcScreen(const PcStats& pcStats);
    void drawMediaScreen(const PcMedia& media);
    void drawBambuScreen(const BambuData& bambu);
    void drawInfoScreen(const unsigned char* image = nullptr, String text = "No Data");

    void drawScreen(int screenIndex, const AppState& state, int subIndex = 0);
    void animateTransition(int prevScreen, int prevSub, int nextScreen, int nextSub, const AppState& state);

    bool isScreenEnabled(const AppState& state, int screenIndex);

private:    
    uint8_t screenBufferOld[1024];
    uint8_t screenBufferNew[1024];

    int getNextAnimationEffect(uint16_t mask);
    void animateHorizontal(int prev, int pSub, int next, int nSub, const AppState& state);
    void animateVertical(int prev, int pSub, int next, int nSub, const AppState& state);
    void animateDissolve(int prev, int pSub, int next, int nSub, const AppState& state);
    void animateCurtain(int prev, int pSub, int next, int nSub, const AppState& state);
    void animateBlinds(int prev, int pSub, int next, int nSub, const AppState& state);

    const unsigned char* getWeatherBitmap(int wmo_code, bool is_day);
    const unsigned char* getAQIBitmap(int val, bool is_eu);
};

#endif