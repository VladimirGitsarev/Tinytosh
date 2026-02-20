#include "CurrencyService.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

bool CurrencyService::fetchRate(const String& base, const String& target, CurrencyData &data) {
    String safeBase = base;
    safeBase.toLowerCase();
    safeBase.trim();
    
    String safeTarget = target;
    safeTarget.toLowerCase();
    safeTarget.trim();

    Serial.printf("CurrencyService: Requesting '%s' to '%s'\n", safeBase.c_str(), safeTarget.c_str()); 

    for (int i = 0; i < 2; i++) {
        String url = String(CURRENCY_API_URLS[i]) + safeBase + ".min.json";
        Serial.printf("CurrencyService: Attempt %d - URL: %s\n", i + 1, url.c_str()); 

        HTTPClient http;
        http.setReuse(false); 
        http.begin(url);
        http.setTimeout(10000); 
        int httpCode = http.GET();

        if (httpCode == 200) {
            String payload = http.getString();

            DynamicJsonDocument filter(256);
            filter["date"] = true;
            filter[safeBase.c_str()][safeTarget.c_str()] = true; 

            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));

            if (!error && doc.containsKey("date") && doc.containsKey(safeBase)) {
                data.base = safeBase;
                data.base.toUpperCase();
                data.target = safeTarget;
                data.target.toUpperCase();
                
                data.date = doc["date"].as<String>();
                data.rate = doc[safeBase][safeTarget].as<float>();
                data.updated = true;
                
                Serial.printf("CurrencyService: Success! %s -> %s = %.6f (Date: %s)\n", 
                              data.base.c_str(), data.target.c_str(), data.rate, data.date.c_str());
                http.end();
                return true; 
            } else {
                Serial.println("CurrencyService: JSON parse failed or missing keys on this endpoint.");
            }
        } else {
            Serial.printf("CurrencyService: API failed, HTTP Code: %d\n", httpCode);
        }
        
        http.end();

        if (i == 0) {
            Serial.println("CurrencyService: Primary API failed. Switching to fallback...");
        }
    }

    Serial.println("CurrencyService: All endpoints failed.");
    return false;
}