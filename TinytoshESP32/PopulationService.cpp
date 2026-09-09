#include "PopulationService.h"
#include <ArduinoJson.h>

PopulationService::PopulationService() {}

bool PopulationService::fetchIndicator(String countryCode, String indicator, long long &valInt, double &valFloat, int &year) {
    HTTPClient http;
    String url = String(POPULATION_API_BASE) + countryCode + "/indicator/" + indicator + "?format=json&mrnev=1";
    
    Serial.println("PopulationService: Fetching data from World Bank -> " + url);
    http.setReuse(false);
    http.begin(url);
    http.setConnectTimeout(5000); 
    http.setTimeout(5000);
    
    int httpCode = http.GET();
    bool success = false;
    
    if (httpCode == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error && doc[1].is<JsonArray>()) {
            JsonObject dataObj = doc[1][0];
            if (!dataObj["value"].isNull()) {
                if (indicator == "SP.POP.TOTL") {
                    valInt = dataObj["value"].as<long long>();
                } else {
                    valFloat = dataObj["value"].as<double>();
                }
                year = dataObj["date"].as<int>();
                success = true;
            }
        } else {
            Serial.printf("PopulationService: JSON parsing failed: %s\n", error.c_str());
        }
    } else {
        Serial.printf("PopulationService: API failed, HTTP Code: %d\n", httpCode);
    }
    
    http.end();
    return success;
}

bool PopulationService::fetchPopulation(const Config& config, PopulationData& data) {
    bool anySuccess = false;
    
    if (config.pop_show_world) {
        long long pop = 0; double growth = 0.0; int year = 0;
        
        if (fetchIndicator("WLD", "SP.POP.TOTL", pop, growth, year)) {
            data.world_pop_base = pop;
            data.world_year = year;
            anySuccess = true;
        }
        if (fetchIndicator("WLD", "SP.POP.GROW", pop, growth, year)) {
            data.world_growth = growth;
        }
    }
    
    if (config.pop_show_country && config.country_code != "") {
        long long pop = 0; double growth = 0.0; int year = 0;
        
        if (fetchIndicator(config.country_code, "SP.POP.TOTL", pop, growth, year)) {
            data.country_pop_base = pop;
            data.country_year = year;
            anySuccess = true;
        }
        if (fetchIndicator(config.country_code, "SP.POP.GROW", pop, growth, year)) {
            data.country_growth = growth;
        }
    }
    
    if (anySuccess) {
        time_t now = time(nullptr);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        data.last_fetch_yday = timeinfo.tm_yday;
        
        Serial.printf("PopulationService: Success! World: %lld (%.2f%%) Country: %lld (%.2f%%)\n", 
            data.world_pop_base, data.world_growth, data.country_pop_base, data.country_growth);
        return true;
    }
    
    return false;
}

long long PopulationService::getLivePopulation(long long basePop, double growth, int year) {
    if (basePop <= 0 || year <= 0) return basePop;
    
    struct tm timeinfo = {0};
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = 0;
    timeinfo.tm_mday = 1;
    
    time_t baseline = mktime(&timeinfo);
    time_t now = time(nullptr);
    
    if (now < baseline) return basePop;
    
    double growth_per_sec = (basePop * (growth / 100.0)) / 31557600.0;
    double diff_sec = difftime(now, baseline);
    
    return basePop + (long long)(diff_sec * growth_per_sec);
}