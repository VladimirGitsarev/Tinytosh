#ifndef WEB_SERVER_SERVICE_H
#define WEB_SERVER_SERVICE_H

#include <WebServer.h>
#include <WiFiManager.h>
#include "structs.h"

typedef void (*ConfigSaveCallback)();

class WebServerService {
public:
    WebServerService(int port, ConfigSaveCallback callback);
    void begin();
    void handleClient();
    void setSharedData(Config* config, WeatherData* weather, AirQualityData* airQualityData, CryptoData* cryptoData, CurrencyData* currencyData, StockData* stockData, PcStats* pcStats);
    
    void handleRoot();
    void handleSave();
    void handleUpdate();

    String generateRootPageContent();
    
    Config getUpdatedConfig() const { return *sharedConfig; } 

private:
    WebServer server;
    WiFiManager wm;
    ConfigSaveCallback saveCallback;
    
    Config* sharedConfig;
    WeatherData* sharedWeather;
    AirQualityData* sharedAirQuality;
    CryptoData* sharedCrypto;
    CurrencyData* sharedCurrency;
    StockData* sharedStock;
    PcStats* sharedPcStats;
    
    String getWeatherIcon(int wmo_code);
    String getCurrentTimeShort(String format);
    String getFullDate();
};

#endif