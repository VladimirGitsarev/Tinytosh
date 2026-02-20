#include "TimeService.h"
#include "zones.h"
#include <ArduinoJson.h>

TimeService::TimeService() {}

bool TimeService::fetchLocationData(Config& config) {
  if (!config.auto_detect) {
    Serial.println("TimeService: Auto-detect is off. Skipping location API."); 
    return true; 
  }
  
  Serial.println("TimeService: Fetching location from ip-api.com..."); 
  HTTPClient http;
  http.begin(LOCATION_API_URL);
  http.setTimeout(10000); 
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error && doc["status"] == "success") {
      config.latitude = doc["lat"].as<float>();
      config.longitude = doc["lon"].as<float>();
      config.timezone = doc["timezone"].as<String>();
      config.city = doc["city"].as<String>(); 
      
      Serial.printf("TimeService: Success! Location: %s (Lat: %.4f, Lon: %.4f), TZ: %s\n", 
                    config.city.c_str(), config.latitude, 
                    config.longitude, config.timezone.c_str());
      return true;
    } else {
      Serial.printf("TimeService: Location JSON parsing failed or status not 'success': %s\n", error.c_str()); 
      return false;
    }
  } else {
    Serial.printf("TimeService: ip-api.com HTTP failed, code: %d\n", httpCode); 
    return false;
  }
}

String TimeService::lookupPosixTimezone(const String& ianaTimezone) {
  DynamicJsonDocument doc(6144); 
  DeserializationError error = deserializeJson(doc, POSIX_TIMEZONE_MAP); 
  if (!error) {
    if (doc.containsKey(ianaTimezone)) {
      return doc[ianaTimezone].as<String>();
    } 
  }
  return "GMT0";
}

void TimeService::syncNTP(const String& ianaTimezone) {
  String posixTimezone = lookupPosixTimezone(ianaTimezone);
  
  Serial.printf("TimeService: Configuring NTP with POSIX rule: %s\n", posixTimezone.c_str()); 
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  setenv("TZ", posixTimezone.c_str(), 1); 
  tzset(); 
  
  Serial.println("TimeService: Waiting for NTP time sync..."); 
  time_t now = 0;
  int retryCount = 0;
  while (now < 1672531200L && retryCount < 20) { 
    delay(500);
    now = time(nullptr);
    retryCount++;
  }

  if (now > 1672531200L) { 
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%a %b %d %H:%M:%S %Y", &timeinfo);
    
    Serial.printf("TimeService: Success! Time synced: %s\n", timeStr);
  } else {
    Serial.println("TimeService: NTP time sync failed or timed out.");
  }
}

String TimeService::getCurrentTimeShort(String format) {
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char time_str[12];
    if (format == "12") {
        strftime(time_str, sizeof(time_str), "%I:%M", &timeinfo);
    } else {
        strftime(time_str, sizeof(time_str), "%H:%M", &timeinfo);
    }
    return String(time_str);
}

String TimeService::getCurrentTime(String format) {
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char time_str[24];
    if (format == "12") {
        strftime(time_str, sizeof(time_str), "%I:%M %d %b", &timeinfo);
    } else {
        strftime(time_str, sizeof(time_str), "%H:%M %d %b", &timeinfo);
    }
    return String(time_str);
}

String TimeService::getFullDate() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return "No Date";
    
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%A, %b %d", &timeinfo);
    return String(buffer);
}