#include "DaylightService.h"
#include "TimeService.h"
#include <ArduinoJson.h>

DaylightService::DaylightService() {}

bool DaylightService::fetchDaylight(const Config& config, DaylightData& data) {
  HTTPClient http;
  String url = String(DAYLIGHT_API_BASE) + "?lat=" + String(config.latitude, 4) + "&lng=" + String(config.longitude, 4) + "&tzid=" + config.timezone;
  
  Serial.println("DaylightService: Fetching daylight data from Sunrise-Sunset -> " + url);
  
  http.setReuse(false);
  http.begin(url);
  http.setConnectTimeout(5000); 
  http.setTimeout(5000);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024); 
    DeserializationError error = deserializeJson(doc, payload);

    if (!error && doc["status"] == "OK") {
      data.sunrise_mins = TimeService::parseTimeToMinsFromMidnight(doc["results"]["sunrise"].as<String>());
      data.sunset_mins = TimeService::parseTimeToMinsFromMidnight(doc["results"]["sunset"].as<String>());
      data.noon_mins = TimeService::parseTimeToMinsFromMidnight(doc["results"]["solar_noon"].as<String>());
      data.length_mins = TimeService::parseDurationToMins(doc["results"]["day_length"].as<String>());
      
      Serial.printf(
        "DaylightService: Success! Sunrise: %d m, Sunset: %d m, Noon: %d m, Length: %d m\n", 
        data.sunrise_mins, data.sunset_mins, data.noon_mins, data.length_mins
      );
      
      time_t now = time(nullptr);
      struct tm timeinfo;
      localtime_r(&now, &timeinfo);
      data.last_fetch_yday = timeinfo.tm_yday;
                    
      http.end();
      return true;
    } 
  } 
  http.end();
  return false;
}