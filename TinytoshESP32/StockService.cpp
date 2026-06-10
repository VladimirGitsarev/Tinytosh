#include "StockService.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

bool StockService::fetchStock(const String& symbol, const String& apiKey, StockData &data) {
    if (apiKey.isEmpty()) {
        Serial.println("StockService: ERROR! Finnhub API key is not set.");
        return false;
    }

    String upperSymbol = symbol;
    upperSymbol.toUpperCase();
    upperSymbol.trim();

    String url = String(STOCK_API_URL) + "?symbol=" + upperSymbol + "&token=" + apiKey;

    Serial.printf("StockService: Requesting Stock Data for '%s'\n", upperSymbol.c_str());

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
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            float currentPrice = doc["c"].as<float>();
            float prevClose    = doc["pc"].as<float>();
            float pctChange    = doc["dp"].as<float>();

            if (currentPrice <= 0) {
                Serial.println("StockService: ERROR! Invalid price data (c=0). Symbol may be wrong or market closed.");
                http.end();
                return false;
            }

            data.symbol = upperSymbol;

            // Look up company name from the local topStocks table
            data.name = upperSymbol;
            for (auto& s : topStocks) {
                if (upperSymbol == String(s.ticker)) {
                    data.name = s.name;
                    break;
                }
            }

            data.price          = currentPrice;
            data.previous_close = prevClose;
            data.percent_change = pctChange;
            data.updated        = true;

            Serial.printf("StockService: Success! %s (%s): $%.2f Change: %+.2f%%\n",
                          data.symbol.c_str(), data.name.c_str(), data.price, data.percent_change);

            http.end();
            return true;
        } else {
            Serial.printf("StockService: JSON parsing failed: %s\n", error.c_str());
        }
    } else {
        Serial.printf("StockService: API failed, HTTP Code: %d\n", httpCode);
    }

    http.end();
    return false;
}
