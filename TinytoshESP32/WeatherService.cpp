#include "WeatherService.h"
#include <ArduinoJson.h>

WeatherService::WeatherService() {}

String WeatherService::getWeatherDescription(int wmo_code) {
    if (wmo_code == 0) return "Clear Sky";
    if (wmo_code >= 1 && wmo_code <= 3) return "Cloudy";
    if (wmo_code >= 45 && wmo_code <= 48) return "Fog";
    if (wmo_code >= 51 && wmo_code <= 67) return "Rain";
    if (wmo_code >= 71 && wmo_code <= 77) return "Snow";
    if (wmo_code >= 95) return "Thunder";
    return "Unknown";
}

String WeatherService::getWeatherIcon(int wmo_code) {
  if (wmo_code == 0) return "☀️"; 
  if (wmo_code == 1 || wmo_code == 2 || wmo_code == 3) return "🌤️"; 
  if (wmo_code <= 48) return "🌫️"; 
  if (wmo_code <= 55) return "🌧️"; 
  if (wmo_code <= 65) return "☔"; 
  if (wmo_code <= 75) return "❄️"; 
  if (wmo_code <= 86) return "🌨️"; 
  if (wmo_code <= 99) return "🌩️"; 
  return "❓";
}

bool WeatherService::isWeatherValid(const WeatherData& data) {
    return !isnan(data.temp) && data.weather_code != -1;
}

bool WeatherService::fetchWeather(const Config& config, WeatherData& data, const String& updateTime) {
  Serial.println("WeatherService: Fetching weather data from Open-Meteo..."); 
  HTTPClient http;
  
  String url = String(WEATHER_API_BASE) + "?latitude=" + String(config.latitude, 4) + 
               "&longitude=" + String(config.longitude, 4) + 
               "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,apparent_temperature,is_day";
  
  Serial.println("WeatherService: Requesting weather data from: " + url); 
  http.begin(url);
  http.setTimeout(10000); 
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(4096); 
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      float temp_c = doc["current"]["temperature_2m"].as<float>();
      float apparent_temp_c = doc["current"]["apparent_temperature"].as<float>();

      if (config.temp_unit == "F") {
          data.temp = temp_c * 1.8 + 32;
          data.apparent_temperature = apparent_temp_c * 1.8 + 32;
      } else {
          data.temp = temp_c;
          data.apparent_temperature = apparent_temp_c;
      }
      
      data.wind_speed = doc["current"]["wind_speed_10m"].as<float>();
      data.humidity = doc["current"]["relative_humidity_2m"].as<int>();
      data.weather_code = doc["current"]["weather_code"].as<int>();
      data.is_day = doc["current"]["is_day"].as<bool>();
      data.update_time = updateTime;
      
      Serial.printf("WeatherService: Weather updated. Temp: %.1f %s\n", data.temp, config.temp_unit.c_str()); 
      http.end();
      return true;
      
    } else {
      Serial.printf("WeatherService: JSON parsing failed: %s\n", error.c_str()); 
      http.end();
      return false;
    }
  } else {
    Serial.printf("WeatherService: Open-Meteo HTTP GET failed, code: %d\n", httpCode); 
    http.end();
    return false;
  }
}