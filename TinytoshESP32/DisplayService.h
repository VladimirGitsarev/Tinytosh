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
    void begin();
    
    void showOLEDStatus(std::initializer_list<String> lines, bool clear = true);
    void drawTimeScreen(const Config& config, String timeStr, String dateStr);
    void drawWeatherScreen(const Config& config, const WeatherData& data, const String& currentTime);
    void drawAQIScreen(const Config& config, const AirQualityData& data, const String& currentTime);
    void drawCryptoScreen(const Config& config, const CryptoData& data);
    void drawCurrencyScreen(const Config& config, const CurrencyData& data);
    void drawStockScreen(const Config& config, const StockData& data);
    void drawPcScreen(const PcStats& pcStats);
    void drawNoData();

    void drawScreen(int screenIndex, const Config& config, TimeService& timeService, const WeatherData& weather, const AirQualityData& aqi, const CryptoData& crypto, const CurrencyData& currency, const StockData& stock, const PcStats& pc);
    void animateTransition(int prevScreen, int nextScreen, const Config& config, TimeService& timeService, const WeatherData& weather, const AirQualityData& aqi, const CryptoData& crypto, const CurrencyData& currency, const StockData& stock, const PcStats& pc);

    bool isScreenEnabled(const Config& config, int screenIndex);

private:    
    uint8_t screenBufferOld[1024];
    uint8_t screenBufferNew[1024];

    int getNextAnimationEffect(uint16_t mask);
    void animateHorizontal(int prev, int next, const Config& c, TimeService& t, const WeatherData& w, const AirQualityData& a, const CryptoData& cr, const CurrencyData& cd, const StockData& s, const PcStats& p);
    void animateVertical(int prev, int next, const Config& c, TimeService& t, const WeatherData& w, const AirQualityData& a, const CryptoData& cr, const CurrencyData& cd, const StockData& s, const PcStats& p);
    void animateDissolve(int prev, int next, const Config& c, TimeService& t, const WeatherData& w, const AirQualityData& a, const CryptoData& cr, const CurrencyData& cd, const StockData& s, const PcStats& p);
    void animateCurtain(int prev, int next, const Config& c, TimeService& t, const WeatherData& w, const AirQualityData& a, const CryptoData& cr, const CurrencyData& cd, const StockData& s, const PcStats& p);
    void animateBlinds(int prev, int next, const Config& c, TimeService& t, const WeatherData& w, const AirQualityData& a, const CryptoData& cr, const CurrencyData& cd, const StockData& s, const PcStats& p);

    String getWeatherDescription(int wmo_code);
    const unsigned char* getWeatherBitmap(int wmo_code, bool is_day);
    const unsigned char* getAQIBitmap(int val, bool is_eu);
};

#endif