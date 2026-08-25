#include "StockService.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

bool StockService::fetchStock(const String& symbol, StockData &data) {
    String safeSymbol = symbol;
    safeSymbol.toUpperCase();
    safeSymbol.trim();

    Serial.printf("StockService: Requesting Stock Data for '%s'\n", safeSymbol.c_str()); 

    WiFiClientSecure client;
    client.setInsecure(); 
    
    for (int i = 0; i < 2; i++) {
        String url = String(STOCK_API_URLS[i]) + safeSymbol;
        Serial.println("StockService: Fetching stock data from Yahoo Finance (Attempt " + String(i + 1) + ") -> " + url);

        HTTPClient http;
        http.setReuse(false); 
        
        http.begin(client, url);
        http.setConnectTimeout(10000); 
        http.setTimeout(10000);        
        
        int httpCode = http.GET();

        if (httpCode == 200) {
            String payload = http.getString();
            DynamicJsonDocument filter(256);
            filter["chart"]["result"][0]["meta"]["symbol"] = true;
            filter["chart"]["result"][0]["meta"]["shortName"] = true;
            filter["chart"]["result"][0]["meta"]["regularMarketPrice"] = true;
            filter["chart"]["result"][0]["meta"]["chartPreviousClose"] = true;

            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));

            if (!error) {
                JsonArray resultArray = doc["chart"]["result"];
                if (!resultArray.isNull() && resultArray.size() > 0) {
                    JsonObject meta = resultArray[0]["meta"];
                    
                    data.symbol = safeSymbol;
                    data.name = meta.containsKey("shortName") ? meta["shortName"].as<String>() : safeSymbol;
                    data.price = meta["regularMarketPrice"].as<float>();
                    data.previous_close = meta["chartPreviousClose"].as<float>();

                    if (data.previous_close > 0) {
                        data.percent_change = ((data.price - data.previous_close) / data.previous_close) * 100.0;
                    } else {
                        data.percent_change = 0.0;
                    }
                    
                    data.updated = true;

                    Serial.printf("StockService: Success! %s (%s): $%.2f Change: %+.2f%%\n", data.symbol.c_str(), data.name.c_str(), data.price, data.percent_change);
                                  
                    http.end();
                    return true;
                } else {
                    Serial.println("StockService: ERROR! 'result' array missing or empty in JSON.");
                }
            } else {
                Serial.printf("StockService: JSON parsing failed: %s\n", error.c_str());
            }
        } else {
            Serial.printf("StockService: API failed, HTTP Code: %d\n", httpCode);
        }
        
        http.end();

        if (i == 0) {
            Serial.println("StockService: Primary API failed. Switching to fallback...");
        }
    }

    Serial.println("StockService: All endpoints failed.");
    return false;
}