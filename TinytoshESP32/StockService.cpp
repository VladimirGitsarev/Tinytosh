#include "StockService.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

bool StockService::fetchStock(const String& symbol, StockData &data) {
    String safeSymbol = symbol;
    safeSymbol.toLowerCase();
    safeSymbol.trim();

    String url = String(STOCK_API_URL) + "?s=" + safeSymbol + ".us&f=cpn&e=json";

    Serial.printf("StockService: Requesting Stock Data for '%s'\n", safeSymbol.c_str()); 
    Serial.printf("StockService: URL: %s\n", url.c_str()); 

    WiFiClientSecure client;
    client.setInsecure(); 
    
    HTTPClient http;
    http.setReuse(false); 
    
    http.begin(client, url);
    http.setConnectTimeout(10000); 
    http.setTimeout(10000);        
    
    int httpCode = http.GET();

    if (httpCode == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            JsonArray symbolsArray = doc["symbols"];
            if (!symbolsArray.isNull() && symbolsArray.size() > 0) {
                JsonObject obj = symbolsArray[0];
                
                // 1. Directly assign the symbol we already know!
                data.symbol = safeSymbol;
                data.symbol.toUpperCase();
                
                // 2. Grab the full company name from the API
                data.name = obj.containsKey("name") ? obj["name"].as<String>() : "Unknown";
                
                // 3. Parse prices
                float currentPrice = obj.containsKey("close") ? obj["close"].as<float>() : obj["c"].as<float>();
                float prevPrice = obj.containsKey("previous") ? obj["previous"].as<float>() : obj["p"].as<float>();

                data.price = currentPrice;
                data.previous_close = prevPrice;

                if (data.previous_close > 0) {
                    data.percent_change = ((data.price - data.previous_close) / data.previous_close) * 100.0;
                } else {
                    data.percent_change = 0.0;
                }
                
                data.updated = true;

                Serial.printf("StockService: Success! %s (%s): $%.2f Change: %+.2f%%\n", 
                              data.symbol.c_str(), data.name.c_str(), data.price, data.percent_change);
                              
                http.end();
                return true;
            } else {
                Serial.println("StockService: ERROR! 'symbols' array missing or empty in JSON.");
            }
        } else {
            Serial.printf("StockService: JSON parsing failed: %s\n", error.c_str());
        }
    } else {
        Serial.printf("StockService: API failed, HTTP Code: %d\n", httpCode);
    }
    
    http.end();
    return false;
}