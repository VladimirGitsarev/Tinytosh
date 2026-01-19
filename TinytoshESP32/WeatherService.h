#ifndef WEATHER_SERVICE_H
#define WEATHER_SERVICE_H

#include "structs.h"
#include <Arduino.h>
#include <HTTPClient.h>

class WeatherService {
public:
    WeatherService();
    
    bool fetchWeather(const Config& config, WeatherData& data, const String& updateTime);
    String getWeatherIcon(int wmo_code);
    String getWeatherDescription(int wmo_code);
    bool isWeatherValid(const WeatherData& data);

private:
    const char* WEATHER_API_BASE = "https://api.open-meteo.com/v1/forecast";
};

#endif