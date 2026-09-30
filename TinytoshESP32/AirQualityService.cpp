#include "AirQualityService.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>


bool AirQualityService::fetchAirQuality(const Config& config, AirQualityData &data) {
  HTTPClient http;
  String typeParam = (config.aqi_type == "EU") ? "european_aqi" : "us_aqi";

  bool wantPm25 = aqiValueSelected(config, "pm25");
  bool wantPm10 = aqiValueSelected(config, "pm10");
  bool wantNo2 = aqiValueSelected(config, "no2");
  bool wantCo = aqiValueSelected(config, "co");
  bool wantCo2 = aqiValueSelected(config, "co2");
  bool wantSo2 = aqiValueSelected(config, "so2");
  bool wantO3 = aqiValueSelected(config, "o3");
  bool wantDust = aqiValueSelected(config, "dust");
  bool wantUv = aqiValueSelected(config, "uv");
  bool wantCh4 = aqiValueSelected(config, "ch4");

  String current = typeParam;
  if (wantPm25) current += ",pm2_5";
  if (wantPm10) current += ",pm10";
  if (wantNo2) current += ",nitrogen_dioxide";
  if (wantCo) current += ",carbon_monoxide";
  if (wantCo2) current += ",carbon_dioxide";
  if (wantSo2) current += ",sulphur_dioxide";
  if (wantO3) current += ",ozone";
  if (wantDust) current += ",dust";
  if (wantUv) current += ",uv_index";
  if (wantCh4) current += ",methane";

  String url = String(AIR_QUALITY_API_URL) + "?latitude=" + String(config.latitude, 4) +
                "&longitude=" + String(config.longitude, 4) +
                "&current=" + current;

  Serial.println("AirQualityService: Fetching air quality data from Open-Meteo -> " + url);

  http.setReuse(false);
  http.begin(url);
  http.setConnectTimeout(5000);
  http.setTimeout(5000);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      JsonObject current = doc["current"];
      data.aqi = current[typeParam].as<int>();
      data.pm25 = wantPm25 ? current["pm2_5"].as<float>() : NAN;
      data.pm10 = wantPm10 ? current["pm10"].as<float>() : NAN;
      data.no2 = wantNo2 ? current["nitrogen_dioxide"].as<float>() : NAN;
      data.co = wantCo ? current["carbon_monoxide"].as<float>() : NAN;
      data.co2 = wantCo2 ? current["carbon_dioxide"].as<float>() : NAN;
      data.so2 = wantSo2 ? current["sulphur_dioxide"].as<float>() : NAN;
      data.o3 = wantO3 ? current["ozone"].as<float>() : NAN;
      data.dust = wantDust ? current["dust"].as<float>() : NAN;
      data.uv = wantUv ? current["uv_index"].as<float>() : NAN;
      data.ch4 = wantCh4 ? current["methane"].as<float>() : NAN;
      data.status = getAQIDescription(data.aqi, config.aqi_type == "EU");

      Serial.printf("AirQualityService: Success! %s AQI: %d (%s)\n", config.aqi_type.c_str(), data.aqi, data.status.c_str());

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

bool AirQualityService::aqiValueSelected(const Config& config, const char* key) {
    for (int i = 0; i < 6; i++) {
        if (config.aqi_values[i] == key) return true;
    }
    return false;
}