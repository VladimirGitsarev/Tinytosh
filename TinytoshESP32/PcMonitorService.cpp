#include <HardwareSerial.h>
#include "PcMonitorService.h"

bool PcMonitorService::handleSerial(AppState &state) {
    bool configUpdated = false;

    while (Serial.available()) {
        String incoming = Serial.readStringUntil('\n');
        incoming.trim();
        
        if (incoming.length() > 0) {
            if (incoming == "GET_UPDATE") {
                sendUpdateOverSerial(state);
            } 
            else if (incoming.startsWith("SAVE_CFG:")) {
                if (parseConfigJson(incoming.substring(9).c_str(), state)) {
                    configUpdated = true;
                }
            } 
            else if (incoming.startsWith("{")) {
                parseJson(incoming.c_str(), state);
            }
        }
    }

    if (millis() - state.pc.last_update > DATA_TIMEOUT_MS) {
        state.pc.cpu_percent = 0;
        state.pc.net_down_kb = 0;
        state.pc.mem_percent = 0;
        state.pc.disk_percent = 0;
    }

    return configUpdated;
}

void PcMonitorService::parseJson(const char* jsonString, AppState &state) {
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, jsonString);

    if (!error) {
        state.pc.cpu_percent = doc["cpu_percent"] | 0.0;
        state.pc.mem_percent = doc["mem_percent"] | 0.0;
        state.pc.disk_percent = doc["disk_percent"] | 0.0;
        state.pc.net_down_kb = doc["net_down_kb"] | 0.0;
        
        String incoming_id = doc["pc_id"] | "";
        if (incoming_id != "") {
            state.config.active_pc_id = incoming_id;
        }
        
        state.pc.last_update = millis(); 
    }
}

void PcMonitorService::sendUpdateOverSerial(AppState &state) {
    DynamicJsonDocument doc(3072);
    Config& config = state.config;

    // Pack Global Settings
    doc["device_id"] = config.device_id;
    doc["ip_address"] = config.ip_address;
    doc["refresh_min"] = config.refresh_interval_min;
    doc["auto_cycle"] = config.screen_auto_cycle ? 1 : 0;
    doc["screen_int"] = config.screen_interval_sec;
    doc["anim_mask"] = config.anim_mask;
    doc["time_format"] = config.time_format;

    // Pack Location & Night Mode
    doc["auto_detect"] = config.auto_detect ? 1 : 0;
    doc["city"] = config.city;
    doc["latitude"] = config.latitude;
    doc["longitude"] = config.longitude;
    doc["timezone"] = config.timezone;
    doc["night_mode"] = config.night_mode ? 1 : 0;
    doc["night_start"] = config.night_start;
    doc["night_end"] = config.night_end;
    doc["night_action"] = config.night_action;

    // Pack Screen Toggles
    doc["show_time"] = config.show_time ? 1 : 0;
    doc["date_display"] = config.date_display ? 1 : 0;
    doc["show_weather"] = config.show_weather ? 1 : 0;
    doc["temp_unit"] = config.temp_unit;
    doc["round_temps"] = config.round_temps ? 1 : 0;
    doc["show_aqi"] = config.show_aqi ? 1 : 0;
    doc["aqi_type"] = config.aqi_type;
    doc["show_pc"] = config.show_pc ? 1 : 0;
    doc["show_stock"] = config.show_stock ? 1 : 0;
    doc["stock_symbol"] = config.stock_symbol;
    doc["stock_fn"] = config.stock_fn ? 1 : 0;
    doc["show_crypto"] = config.show_crypto ? 1 : 0;
    doc["crypto_id"] = config.crypto_id;
    doc["crypto_fn"] = config.crypto_fn ? 1 : 0;
    doc["show_currency"] = config.show_currency ? 1 : 0;
    doc["currency_base"] = config.currency_base;
    doc["currency_target"] = config.currency_target;
    doc["currency_multiplier"] = config.currency_multiplier;
    doc["currency_fn"] = config.currency_fn ? 1 : 0;

    String orderStr = "";
    for(int i = 0; i < NUM_SCREENS; i++) {
        orderStr += String(config.screen_order[i]);
        if(i < NUM_SCREENS - 1) orderStr += ",";
    }
    doc["screen_order"] = orderStr;

    WeatherData& weather = state.weather;
    AirQualityData& aqi = state.aqi;
    CryptoData& crypto = state.crypto;
    CurrencyData& currency = state.currency;
    StockData& stock = state.stock;
    PcStats& pc = state.pc;

    doc["update_time"] = weather.update_time;
    if (!isnan(weather.temp)) {
        doc["temp"] = String(weather.temp, 1);
        doc["apparent_temperature"] = String(weather.apparent_temperature, 1);
        doc["humidity"] = String(weather.humidity);
        doc["wind_speed"] = String(weather.wind_speed, 1);
        doc["temp_unit"] = config.temp_unit;
    }

    if (!isnan(aqi.pm25) && !isnan(aqi.pm10) && !isnan(aqi.no2)) {
        doc["aqi"] = String(aqi.aqi);
        doc["aqi_status"] = aqi.status;
        doc["pm25"] = String(aqi.pm25, 1);
        doc["pm10"] = String(aqi.pm10, 1);
        doc["no2"] = String(aqi.no2, 1);
    }

    if (!isnan(crypto.price_usd) && crypto.price_usd > 0) {
        doc["crypto_price"] = String(crypto.price_usd);
        doc["crypto_change"] = String(crypto.percent_change_24h);
    }

    if (currency.updated) {
        float displayRate = currency.rate * config.currency_multiplier;
        int decimals = (displayRate < 10.0) ? 3 : ((displayRate < 100.0) ? 2 : 1);
        doc["currency_base_text"] = String(config.currency_multiplier) + " " + currency.base;
        doc["currency_target_text"] = String(displayRate, decimals) + " " + currency.target;
    }

    if (stock.updated) {
        doc["stock_symbol"] = stock.symbol;
        doc["stock_price"] = String(stock.price, 2);
        doc["stock_change"] = String(stock.percent_change, 2);
    }

    String activeId = config.active_pc_id;
    int lastDashSync = activeId.lastIndexOf(':');
    if (lastDashSync > 3) activeId = activeId.substring(0, lastDashSync);
    bool isPcPaired = (activeId != "" && (millis() - pc.last_update < 5000));
    doc["pc_status"] = isPcPaired ? ("🔒 Paired to " + activeId) : "";

    String jsonResponse;
    serializeJson(doc, jsonResponse);
    Serial.print("SYS_UPDATE:");
    Serial.println(jsonResponse);
}

bool PcMonitorService::parseConfigJson(const char* jsonString, AppState &state) {
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, jsonString);
    
    if (error) {
        Serial.println("SYS_MSG:Failed to parse incoming config");
        return false;
    }

    Config& config = state.config;

    if (doc.containsKey("refresh_min")) config.refresh_interval_min = doc["refresh_min"];
    if (doc.containsKey("auto_cycle")) config.screen_auto_cycle = doc["auto_cycle"] == 1;
    if (doc.containsKey("screen_int")) config.screen_interval_sec = doc["screen_int"];
    if (doc.containsKey("anim_mask")) config.anim_mask = doc["anim_mask"];
    if (doc.containsKey("time_format")) config.time_format = doc["time_format"].as<String>();
    
    if (doc.containsKey("auto_detect")) config.auto_detect = doc["auto_detect"] == 1;
    if (doc.containsKey("city")) config.city = doc["city"].as<String>();
    if (doc.containsKey("latitude")) config.latitude = doc["latitude"].as<float>();
    if (doc.containsKey("longitude")) config.longitude = doc["longitude"].as<float>();
    if (doc.containsKey("timezone")) config.timezone = doc["timezone"].as<String>();

    if (doc.containsKey("night_mode")) config.night_mode = doc["night_mode"] == 1;
    if (doc.containsKey("night_start")) config.night_start = doc["night_start"].as<String>();
    if (doc.containsKey("night_end")) config.night_end = doc["night_end"].as<String>();
    if (doc.containsKey("night_action")) config.night_action = doc["night_action"];

    if (doc.containsKey("show_time")) config.show_time = doc["show_time"] == 1;
    if (doc.containsKey("date_display")) config.date_display = doc["date_display"] == 1;
    if (doc.containsKey("show_weather")) config.show_weather = doc["show_weather"] == 1;
    if (doc.containsKey("temp_unit")) config.temp_unit = doc["temp_unit"].as<String>();
    if (doc.containsKey("round_temps")) config.round_temps = doc["round_temps"] == 1;
    if (doc.containsKey("show_aqi")) config.show_aqi = doc["show_aqi"] == 1;
    if (doc.containsKey("aqi_type")) config.aqi_type = doc["aqi_type"].as<String>();
    
    if (doc.containsKey("show_pc")) config.show_pc = doc["show_pc"] == 1;
    if (doc.containsKey("show_stock")) config.show_stock = doc["show_stock"] == 1;
    if (doc.containsKey("stock_symbol")) config.stock_symbol = doc["stock_symbol"].as<String>();
    if (doc.containsKey("stock_fn")) config.stock_fn = doc["stock_fn"] == 1;
    
    if (doc.containsKey("show_crypto")) config.show_crypto = doc["show_crypto"] == 1;
    if (doc.containsKey("crypto_id")) config.crypto_id = doc["crypto_id"].as<int>();
    if (doc.containsKey("crypto_fn")) config.crypto_fn = doc["crypto_fn"] == 1;
    
    if (doc.containsKey("show_currency")) config.show_currency = doc["show_currency"] == 1;
    if (doc.containsKey("currency_base")) config.currency_base = doc["currency_base"].as<String>();
    if (doc.containsKey("currency_target")) config.currency_target = doc["currency_target"].as<String>();
    if (doc.containsKey("currency_multiplier")) config.currency_multiplier = doc["currency_multiplier"];
    if (doc.containsKey("currency_fn")) config.currency_fn = doc["currency_fn"] == 1;

    if (doc.containsKey("screen_order")) {
        String orderStr = doc["screen_order"].as<String>();
        int idx = 0;
        int startPos = 0;
        while (startPos < orderStr.length() && idx < NUM_SCREENS) {
            int commaPos = orderStr.indexOf(',', startPos);
            if (commaPos == -1) {
                config.screen_order[idx++] = orderStr.substring(startPos).toInt();
                break;
            } else {
                config.screen_order[idx++] = orderStr.substring(startPos, commaPos).toInt();
                startPos = commaPos + 1;
            }
        }
    }

    Serial.println("SYS_MSG:Settings Saved Successfully");
    return true;
}

const PcStats& PcMonitorService::getStats() const {
    return currentStats;
}