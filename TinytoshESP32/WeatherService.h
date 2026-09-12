#ifndef WEATHER_SERVICE_H
#define WEATHER_SERVICE_H

#include <Arduino.h>

#include "structs.h"

class WeatherService {
public:
    WeatherService();
    
    bool fetchWeather(const Config& config, WeatherData& data, const String& updateTime);
    static String getWeatherIcon(int wmo_code);
    static String getWeatherDescription(int wmo_code);
    static bool isWeatherValid(const WeatherData& data);

private:
    static constexpr const char* WEATHER_API_BASE = "https://api.open-meteo.com/v1/forecast";
};

#endif