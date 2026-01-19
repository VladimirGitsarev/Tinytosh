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
    void setSharedData(Config* config, WeatherData* weather, PcStats* pcStats, CryptoData* cryptoData, AirQualityData* airQualityData);
    
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
    PcStats* sharedPcStats;
    CryptoData* sharedCrypto;
    AirQualityData* sharedAirQuality;
    
    String getWeatherIcon(int wmo_code);
    String getCurrentTimeShort(String format);
    String getFullDate();
};

#endif