#include "AirQualityService.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>

bool AirQualityService::fetchAirQuality(const Config& config, AirQualityData &data) {
  Serial.println("AirQualityService: Fetching Air Quality data from Open-Meteo..."); 
  HTTPClient http;

  String typeParam = (config.aqi_type == "EU") ? "european_aqi" : "us_aqi";
  
  String url = String(AIR_QUALITY_API_URL) + "?latitude=" + String(config.latitude, 4) + 
               "&longitude=" + String(config.longitude, 4) + 
               "&current=pm2_5,pm10,nitrogen_dioxide," + typeParam;
  
  Serial.println("AirQualityService: Requesting Air Quality data from: " + url); 
  http.begin(url);
  http.setTimeout(10000); 
  int httpCode = http.GET();

  if (httpCode == 200) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, http.getString());
    
    JsonObject current = doc["current"];
    data.aqi = current[typeParam];
    data.pm25 = current["pm2_5"];
    data.pm10 = current["pm10"];
    data.no2 = current["nitrogen_dioxide"];
    data.status = getAQIDescription(data.aqi, config.aqi_type == "EU");
    
    http.end();
    return true;
  }
  http.end();
  return false;
}

String AirQualityService::getAQIDescription(int val, bool is_eu) {
    if (is_eu) {
        if (val <= 20)  return "Good";
        if (val <= 40)  return "Fair";
        if (val <= 60)  return "Moderate";
        if (val <= 80)  return "Poor";
        if (val <= 100) return "Very Poor";
        return "Extreme";
    } else {
        if (val <= 50)  return "Good";
        if (val <= 100) return "Moderate";
        if (val <= 150) return "Sensitive";
        if (val <= 200) return "Unhealthy";
        if (val <= 300) return "V. Unhealthy";
        return "Hazardous";
    }
}