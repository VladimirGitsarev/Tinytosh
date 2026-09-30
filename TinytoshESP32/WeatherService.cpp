#include "WeatherService.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>

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

bool WeatherService::weatherValueSelected(const Config& config, const char* key) {
    for (int i = 0; i < 6; i++) {
        if (config.weather_values[i] == key) return true;
    }
    return false;
}

bool WeatherService::fetchWeather(const Config& config, WeatherData& data, const String& updateTime) {
  HTTPClient http;

  String current = "temperature_2m,weather_code,is_day";
  bool wantFeels = weatherValueSelected(config, "feels");
  bool wantHumidity = weatherValueSelected(config, "humidity");
  bool wantWind = weatherValueSelected(config, "wind");
  bool wantPrecipitation = weatherValueSelected(config, "precipitation");
  bool wantPressure = weatherValueSelected(config, "pressure");
  bool wantVisibility = weatherValueSelected(config, "visibility");
  if (wantFeels) current += ",apparent_temperature";
  if (wantHumidity) current += ",relative_humidity_2m";
  if (wantWind) current += ",wind_speed_10m";
  if (wantPrecipitation) current += ",precipitation_probability";
  if (wantPressure) current += ",surface_pressure";
  if (wantVisibility) current += ",visibility";

  String url = String(WEATHER_API_BASE) + "?latitude=" + String(config.latitude, 4) +
                "&longitude=" + String(config.longitude, 4) +
                "&current=" + current;

  Serial.println("WeatherService: Fetching weather data from Open-Meteo -> " + url);

  http.setReuse(false);
  http.begin(url);
  http.setConnectTimeout(5000);
  http.setTimeout(5000);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      JsonObject cur = doc["current"];
      float temp_c = cur["temperature_2m"].as<float>();
      data.temp = (config.temp_unit == "F") ? temp_c * 1.8 + 32 : temp_c;

      if (wantFeels) {
          float apparent_temp_c = cur["apparent_temperature"].as<float>();
          data.apparent_temperature = (config.temp_unit == "F") ? apparent_temp_c * 1.8 + 32 : apparent_temp_c;
      } else {
          data.apparent_temperature = NAN;
      }
      data.humidity = wantHumidity ? cur["relative_humidity_2m"].as<int>() : 0;
      data.wind_speed = wantWind ? cur["wind_speed_10m"].as<float>() : NAN;
      data.precipitation_probability = wantPrecipitation ? cur["precipitation_probability"].as<float>() : NAN;
      data.pressure = wantPressure ? cur["surface_pressure"].as<float>() : NAN;
      data.visibility = wantVisibility ? cur["visibility"].as<float>() : NAN;

      data.weather_code = cur["weather_code"].as<int>();
      data.is_day = cur["is_day"].as<bool>();
      data.update_time = updateTime;

      Serial.printf("WeatherService: Success! Temp: %.1f%s, Code: %d\n", data.temp, config.temp_unit.c_str(), data.weather_code);

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