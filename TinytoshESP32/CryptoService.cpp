#include "CryptoService.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

bool CryptoService::fetchPrice(int id, CryptoData &data) {
        HTTPClient http;
        String url = String(CRYPTO_API_URL) + "?id=" + String(id);

        Serial.printf("CryptoService: Requesting Crypto Data from CoinLore: %d\n", id); 
        
        http.begin(url);
        int httpCode = http.GET();

        if (httpCode == 200) {
            String payload = http.getString();
            StaticJsonDocument<512> doc;
            deserializeJson(doc, payload);

            JsonObject obj = doc[0];
            data.name = obj["name"].as<String>();
            data.symbol = obj["symbol"].as<String>();
            data.price_usd = obj["price_usd"].as<float>();
            data.percent_change_24h = obj["percent_change_24h"].as<float>();
            data.updated = true;
            return true;
        }
        return false;
    }