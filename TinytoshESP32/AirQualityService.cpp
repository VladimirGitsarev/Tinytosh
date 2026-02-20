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
  http.setReuse(false); 
  http.begin(url);
  http.setTimeout(10000); 
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      JsonObject current = doc["current"];
      data.aqi = current[typeParam].as<int>();
      data.pm25 = current["pm2_5"].as<float>();
      data.pm10 = current["pm10"].as<float>();
      data.no2 = current["nitrogen_dioxide"].as<float>();
      data.status = getAQIDescription(data.aqi, config.aqi_type == "EU");
      
      Serial.printf("AirQualityService: Success! %s AQI: %d (%s), PM2.5: %.1f, PM10: %.1f, NO2: %.1f\n", 
                    config.aqi_type.c_str(), data.aqi, data.status.c_str(), 
                    data.pm25, data.pm10, data.no2);
                    
      http.end();
      return true;
    } else {
      Serial.printf("AirQualityService: JSON parsing failed: %s\n", error.c_str());
    }
  } else {
    Serial.printf("AirQualityService: API failed, HTTP Code: %d\n", httpCode);
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