#include "MoonService.h"
#include "TimeService.h"
#include <ArduinoJson.h>

MoonService::MoonService() {}

bool MoonService::fetchMoon(const Config& config, MoonData& data) {
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  
  char dateBuf[16];
  strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d", &timeinfo);
  String dateStr = String(dateBuf);
  
  struct tm utc_tm;
  gmtime_r(&now, &utc_tm);
  
  time_t local_as_utc = mktime(&utc_tm);
  float tz_offset = difftime(now, local_as_utc) / 3600.0;

  HTTPClient http;
  String url = String(MOON_API_BASE) + "?date=" + dateStr + "&coords=" + String(config.latitude, 4) + "," + String(config.longitude, 4) + "&tz=" + String(tz_offset, 1);

  Serial.println("MoonService: Fetching moon data from USNO -> " + url);
  
  http.setReuse(false);
  http.begin(url);
  http.setConnectTimeout(5000);
  http.setTimeout(5000);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error && !doc["error"].as<bool>()) {
      JsonObject dataObj = doc["properties"]["data"];
      data.curphase = dataObj["curphase"].as<String>();
      data.fracillum = dataObj["fracillum"].as<String>().toInt();
      
      data.rise_mins = -1;
      data.set_mins = -1;

      JsonArray moondata = dataObj["moondata"].as<JsonArray>();
      for (JsonObject m : moondata) {
        String phen = m["phen"].as<String>();
        if (phen == "Rise") data.rise_mins = TimeService::parseTimeToMinsFromMidnight(m["time"].as<String>(), "24");
        if (phen == "Set") data.set_mins = TimeService::parseTimeToMinsFromMidnight(m["time"].as<String>(), "24");
      }

      data.last_fetch_yday = timeinfo.tm_yday;

      Serial.printf("MoonService: Success! Phase: %s, Illum: %d%%, Rise: %d m, Set: %d m\n", data.curphase.c_str(), data.fracillum, data.rise_mins, data.set_mins);

      http.end();
      return true;
    } else {
      Serial.println("MoonService: API error or JSON parsing failed.");
    }
  } else {
    Serial.printf("MoonService: HTTP GET failed, code: %d\n", httpCode);
  }

  http.end();
  return false;
}