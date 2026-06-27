#include "JsonSerializer.h"
#include "TimeService.h"

void JsonSerializer::populateConfigDoc(const Config& config, DynamicJsonDocument& doc) {
    doc["device_id"] = config.device_id;
    doc["ip_address"] = config.ip_address;

    doc["sda_pin"] = config.sda_pin;
    doc["scl_pin"] = config.scl_pin;
    doc["touch_pin"] = config.touch_pin;

    doc["refresh_min"] = config.refresh_interval_min;
    doc["auto_cycle"] = config.screen_auto_cycle ? 1 : 0;
    doc["screen_int"] = config.screen_interval_sec;
    doc["anim_mask"] = config.anim_mask;
    doc["time_format"] = config.time_format;
    
    doc["auto_detect"] = config.auto_detect ? 1 : 0;
    doc["country"] = config.country;
    doc["country_code"] = config.country_code;
    doc["city"] = config.city;
    doc["latitude"] = config.latitude;
    doc["longitude"] = config.longitude;
    doc["timezone"] = config.timezone;

    doc["theme_bg"] = config.theme_bg;
    doc["theme_card"] = config.theme_card;
    doc["theme_accent"] = config.theme_accent;
    doc["theme_text"] = config.theme_text;

    doc["night_mode"] = config.night_mode ? 1 : 0;
    doc["night_start"] = config.night_start;
    doc["night_end"] = config.night_end;
    doc["night_action"] = config.night_action;
    doc["night_dim_start"] = config.night_dim_start;

    doc["show_time"] = config.show_time ? 1 : 0;
    doc["date_display"] = config.date_display ? 1 : 0;

    doc["show_calendar"] = config.show_calendar ? 1 : 0;
    doc["cal_start"] = config.calendar_start_day;
    doc["cal_hol"] = config.calendar_show_holidays ? 1 : 0;
    doc["cal_min"] = config.calendar_minimal ? 1 : 0;
    
   doc["show_weather"] = config.show_weather ? 1 : 0;
    doc["temp_unit"] = config.temp_unit;
    doc["round_temps"] = config.round_temps ? 1 : 0;
    doc["weather_hide_bar"] = config.weather_hide_bar ? 1 : 0;
    
    doc["show_aqi"] = config.show_aqi ? 1 : 0;
    doc["aqi_type"] = config.aqi_type;
    doc["aqi_hide_bar"] = config.aqi_hide_bar ? 1 : 0;

    doc["show_daylight"] = config.show_daylight ? 1 : 0;
    doc["daylight_min"] = config.daylight_minimal ? 1 : 0;
    
    doc["show_pc"] = config.show_pc ? 1 : 0;
    doc["show_media"] = config.show_media ? 1 : 0;
    
    // Arrays
    doc["show_stock"] = config.show_stock ? 1 : 0;
    doc["stock_fn"] = config.stock_fn ? 1 : 0;
    JsonArray stArr = doc.createNestedArray("stock_symbols");
    for(int i=0; i<config.stock_count; i++) stArr.add(config.stock_symbols[i]);
    
    doc["show_crypto"] = config.show_crypto ? 1 : 0;
    doc["crypto_fn"] = config.crypto_fn ? 1 : 0;
    JsonArray crArr = doc.createNestedArray("crypto_ids");
    for(int i=0; i<config.crypto_count; i++) crArr.add(config.crypto_ids[i]);

    doc["show_currency"] = config.show_currency ? 1 : 0;
    doc["currency_fn"] = config.currency_fn ? 1 : 0;
    JsonArray cbArr = doc.createNestedArray("currency_bases");
    for(int i=0; i<config.currency_count; i++) cbArr.add(config.currency_bases[i]);
    JsonArray ctArr = doc.createNestedArray("currency_targets");
    for(int i=0; i<config.currency_count; i++) ctArr.add(config.currency_targets[i]);
    JsonArray cmArr = doc.createNestedArray("currency_multipliers");
    for(int i=0; i<config.currency_count; i++) cmArr.add(config.currency_multipliers[i]);

    doc["show_bambu"] = config.show_bambu ? 1 : 0;
    doc["bambu_ip"] = config.bambu_ip;
    doc["bambu_sn"] = config.bambu_sn;
    doc["bambu_code"] = config.bambu_code;

    doc["show_focus"] = config.show_focus ? 1 : 0;
    JsonArray fdArr = doc.createNestedArray("focus_durations");
    for(int i=0; i<config.focus_duration_count; i++) fdArr.add(config.focus_durations[i]);
    doc["focus_flash"] = config.focus_flash ? 1 : 0;

    doc["hide_empty_pc"] = config.hide_empty_pc ? 1 : 0;
    doc["hide_empty_media"] = config.hide_empty_media ? 1 : 0;
    doc["hide_empty_bambu"] = config.hide_empty_bambu ? 1 : 0;

    String orderStr = "";
    for(int i = 0; i < NUM_SCREENS; i++) {
        orderStr += String(config.screen_order[i]);
        if(i < NUM_SCREENS - 1) orderStr += ",";
    }
    doc["screen_order"] = orderStr;
}

String JsonSerializer::buildConfigJson(const Config& config) {
    DynamicJsonDocument doc(2048);
    populateConfigDoc(config, doc);
    String output;
    serializeJson(doc, output);
    return output;
}

String JsonSerializer::buildAppStateJson(const AppState& state) {
    DynamicJsonDocument doc(4096);
    
    // 1. Unload all config variables
    populateConfigDoc(state.config, doc);

    // 2. Unload dynamic state arrays
    doc["time"] = TimeService::getCurrentTimeShort(state.config.time_format);
    doc["date"] = TimeService::getFullDate();
    doc["cal_count"] = state.calendar.count;
    doc["update_time"] = state.weather.update_time;

    if (!isnan(state.weather.temp)) {
        doc["temp"] = String(state.weather.temp, 1);
        doc["apparent_temperature"] = String(state.weather.apparent_temperature, 1);
        doc["humidity"] = String(state.weather.humidity);
        doc["wind_speed"] = String(state.weather.wind_speed, 1);
        doc["weather_code"] = state.weather.weather_code;
    }

    if (!isnan(state.aqi.pm25)) {
        doc["aqi"] = String(state.aqi.aqi);
        doc["aqi_status"] = state.aqi.status;
        doc["pm25"] = String(state.aqi.pm25, 1);
        doc["pm10"] = String(state.aqi.pm10, 1);
        doc["no2"] = String(state.aqi.no2, 1);
    }

    if (state.daylight.sunrise_mins != -1) {
        doc["sunrise"] = TimeService::formatMinsFromMidnight(state.daylight.sunrise_mins, state.config.time_format);
        doc["sunset"] = TimeService::formatMinsFromMidnight(state.daylight.sunset_mins, state.config.time_format);
        doc["solar_noon"] = TimeService::formatMinsFromMidnight(state.daylight.noon_mins, state.config.time_format);
        doc["day_length"] = TimeService::formatDurationMins(state.daylight.length_mins);
    }

    JsonArray stockData = doc.createNestedArray("stock_data");
    for(int i=0; i<state.config.stock_count; i++) {
        if (state.stocks[i].updated) {
            JsonObject obj = stockData.createNestedObject();
            obj["symbol"] = state.stocks[i].symbol;
            obj["price"] = String(state.stocks[i].price, 2);
            obj["change"] = String(state.stocks[i].percent_change, 2);
        }
    }

    JsonArray cryptoData = doc.createNestedArray("crypto_data");
    for(int i=0; i<state.config.crypto_count; i++) {
        if (state.cryptos[i].updated) {
            JsonObject obj = cryptoData.createNestedObject();
            obj["symbol"] = String(state.cryptos[i].symbol);
            obj["price"] = String(state.cryptos[i].price_usd);
            obj["change"] = String(state.cryptos[i].percent_change_24h, 1);
        }
    }

    JsonArray currencyData = doc.createNestedArray("currency_data");
    for(int i=0; i<state.config.currency_count; i++) {
        if (state.currencies[i].updated) {
            JsonObject obj = currencyData.createNestedObject();
            float displayRate = state.currencies[i].rate * state.config.currency_multipliers[i];
            int decimals = (displayRate < 10.0) ? 3 : (displayRate < 100.0) ? 2 : (displayRate < 1000.0) ? 1 : 0;
            obj["base_text"] = String(state.config.currency_multipliers[i]) + " " + state.currencies[i].base;
            obj["target_text"] = String(displayRate, decimals) + " " + state.currencies[i].target;
        }
    }

    if (state.pc.cpu_percent > 0.1) {
        doc["pc_cpu"] = String(state.pc.cpu_percent);
        doc["pc_net"] = String(state.pc.net_down_kb);
        doc["pc_ram"] = String(state.pc.mem_percent);
        doc["pc_disk"] = String(state.pc.disk_percent);
    }

    if (state.media.status.length() > 0) {
        doc["media_status"] = state.media.status;
        doc["media_name"] = state.media.name;
        doc["media_author"] = state.media.author;
        doc["media_album"] = state.media.album;
    }

    if (state.bambu.status != "SYNCING") {
        doc["bambu_status"] = state.bambu.status;
        doc["bambu_progress"] = state.bambu.progress;
        doc["bambu_time"] = state.bambu.time_left;
        doc["bambu_layer"] = state.bambu.layer;
        doc["bambu_total_layers"] = state.bambu.total_layers;
        doc["bambu_nozzle"] = String(state.bambu.nozzle_temp, 1);
        doc["bambu_nozzle_target"] = String(state.bambu.nozzle_target, 1);
        doc["bambu_bed"] = String(state.bambu.bed_temp, 1);
        doc["bambu_bed_target"] = String(state.bambu.bed_target, 1);
        doc["bambu_fan_part"] = state.bambu.fan_part;
        doc["bambu_fan_aux"] = state.bambu.fan_aux;
    }

    String activeId = state.config.active_pc_id;
    int lastDashSync = activeId.lastIndexOf(':');
    if (lastDashSync > 3) activeId = activeId.substring(0, lastDashSync);
    bool isPcPaired = (activeId != "" && (millis() - state.pc.last_update < 10000));
    doc["pc_status"] = isPcPaired ? ("🔒 Paired to " + activeId) : "";

    String output;
    serializeJson(doc, output);
    return output;
}

bool JsonSerializer::parseConfig(const char* jsonString, AppState& state) {
    DynamicJsonDocument doc(3072);
    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) return false;

    Config& config = state.config;

    if (doc.containsKey("sda_pin")) config.sda_pin = doc["sda_pin"];
    if (doc.containsKey("scl_pin")) config.scl_pin = doc["scl_pin"];
    if (doc.containsKey("touch_pin")) config.touch_pin = doc["touch_pin"];

    if (doc.containsKey("refresh_min")) config.refresh_interval_min = doc["refresh_min"];
    if (doc.containsKey("auto_cycle")) config.screen_auto_cycle = doc["auto_cycle"] == 1;
    if (doc.containsKey("screen_int")) config.screen_interval_sec = doc["screen_int"];
    if (doc.containsKey("anim_mask")) config.anim_mask = doc["anim_mask"];
    if (doc.containsKey("time_format")) config.time_format = doc["time_format"].as<String>();
    
    if (doc.containsKey("auto_detect")) config.auto_detect = doc["auto_detect"] == 1;
    if (doc.containsKey("latitude")) config.latitude = doc["latitude"].as<float>();
    if (doc.containsKey("longitude")) config.longitude = doc["longitude"].as<float>();
    if (doc.containsKey("country")) config.country = doc["country"].as<String>();
    if (doc.containsKey("city")) config.city = doc["city"].as<String>();
    if (doc.containsKey("timezone")) config.timezone = doc["timezone"].as<String>();

    if (doc.containsKey("theme_bg")) config.theme_bg = doc["theme_bg"].as<String>();
    if (doc.containsKey("theme_card")) config.theme_card = doc["theme_card"].as<String>();
    if (doc.containsKey("theme_accent")) config.theme_accent = doc["theme_accent"].as<String>();
    if (doc.containsKey("theme_text")) config.theme_text = doc["theme_text"].as<String>();
    
    if (doc.containsKey("country_code")) {
        config.country_code = doc["country_code"].as<String>();
        for (auto c : allCountries) {
            if (config.country_code == String(c.code)) {
                config.country = c.name;
                break;
            }
        }
    }

    if (doc.containsKey("night_mode")) config.night_mode = doc["night_mode"] == 1;
    if (doc.containsKey("night_start")) config.night_start = doc["night_start"].as<String>();
    if (doc.containsKey("night_end")) config.night_end = doc["night_end"].as<String>();
    if (doc.containsKey("night_dim_start")) config.night_dim_start = doc["night_dim_start"].as<String>();
    if (doc.containsKey("night_action")) config.night_action = doc["night_action"];

    if (doc.containsKey("show_time")) config.show_time = doc["show_time"] == 1;
    if (doc.containsKey("date_display")) config.date_display = doc["date_display"] == 1;

    if (doc.containsKey("show_calendar")) config.show_calendar = doc["show_calendar"] == 1;
    if (doc.containsKey("cal_start")) config.calendar_start_day = doc["cal_start"].as<String>();
    if (doc.containsKey("cal_hol")) config.calendar_show_holidays = doc["cal_hol"] == 1;
    if (doc.containsKey("cal_min")) config.calendar_minimal = doc["cal_min"] == 1;

    if (doc.containsKey("show_weather")) config.show_weather = doc["show_weather"] == 1;
    if (doc.containsKey("temp_unit")) config.temp_unit = doc["temp_unit"].as<String>();
    if (doc.containsKey("round_temps")) config.round_temps = doc["round_temps"] == 1;
    if (doc.containsKey("weather_hide_bar")) config.weather_hide_bar = doc["weather_hide_bar"] == 1;

    if (doc.containsKey("show_aqi")) config.show_aqi = doc["show_aqi"] == 1;
    if (doc.containsKey("aqi_type")) config.aqi_type = doc["aqi_type"].as<String>();
    if (doc.containsKey("aqi_hide_bar")) config.aqi_hide_bar = doc["aqi_hide_bar"] == 1;
    
    if (doc.containsKey("show_daylight")) config.show_daylight = doc["show_daylight"] == 1;
    if (doc.containsKey("daylight_min")) config.daylight_minimal = doc["daylight_min"] == 1;
    
    if (doc.containsKey("show_pc")) config.show_pc = doc["show_pc"] == 1;
    
    if (doc.containsKey("show_stock")) config.show_stock = doc["show_stock"] == 1;
    if (doc.containsKey("stock_fn")) config.stock_fn = doc["stock_fn"] == 1;
    if (doc.containsKey("stock_symbols")) {
        JsonArray arr = doc["stock_symbols"].as<JsonArray>();
        config.stock_count = 0;
        for (JsonVariant v : arr) if (config.stock_count < MAX_MULTI_ENTRIES) config.stock_symbols[config.stock_count++] = v.as<String>();
        if (config.stock_count == 0) { config.stock_symbols[0] = "AAPL"; config.stock_count = 1; }
    }
    
    if (doc.containsKey("show_crypto")) config.show_crypto = doc["show_crypto"] == 1;
    if (doc.containsKey("crypto_fn")) config.crypto_fn = doc["crypto_fn"] == 1;
    if (doc.containsKey("crypto_ids")) {
        JsonArray arr = doc["crypto_ids"].as<JsonArray>();
        config.crypto_count = 0;
        for (JsonVariant v : arr) if (config.crypto_count < MAX_MULTI_ENTRIES) config.crypto_ids[config.crypto_count++] = v.as<int>();
        if (config.crypto_count == 0) { config.crypto_ids[0] = 90; config.crypto_count = 1; }
    }
    
    if (doc.containsKey("show_currency")) config.show_currency = doc["show_currency"] == 1;
    if (doc.containsKey("currency_fn")) config.currency_fn = doc["currency_fn"] == 1;
    if (doc.containsKey("currency_bases")) {
        JsonArray arrB = doc["currency_bases"].as<JsonArray>();
        JsonArray arrT = doc["currency_targets"].as<JsonArray>();
        JsonArray arrM = doc["currency_multipliers"].as<JsonArray>();
        config.currency_count = 0;
        for (int i = 0; i < arrB.size(); i++) {
            if (config.currency_count < MAX_MULTI_ENTRIES) {
                config.currency_bases[config.currency_count] = arrB[i].as<String>();
                config.currency_targets[config.currency_count] = arrT[i].as<String>();
                config.currency_multipliers[config.currency_count] = arrM[i].as<int>();
                config.currency_count++;
            }
        }
        if (config.currency_count == 0) { config.currency_bases[0] = "usd"; config.currency_targets[0] = "eur"; config.currency_multipliers[0] = 1; config.currency_count = 1; }
    }

    if (doc.containsKey("show_media")) config.show_media = doc["show_media"] == 1;

    if (doc.containsKey("show_bambu")) config.show_bambu = doc["show_bambu"] == 1;
    if (doc.containsKey("bambu_ip")) config.bambu_ip = doc["bambu_ip"].as<String>();
    if (doc.containsKey("bambu_sn")) config.bambu_sn = doc["bambu_sn"].as<String>();
    if (doc.containsKey("bambu_code")) config.bambu_code = doc["bambu_code"].as<String>();

    if (doc.containsKey("show_focus")) config.show_focus = doc["show_focus"] == 1;
    if (doc.containsKey("focus_durations")) {
        JsonArray arr = doc["focus_durations"].as<JsonArray>();
        config.focus_duration_count = 0;
        for (JsonVariant v : arr) {
            int mins = v.as<int>();
            if (mins > 0 && config.focus_duration_count < MAX_MULTI_ENTRIES) {
                config.focus_durations[config.focus_duration_count++] = mins;
            }
        }
        if (config.focus_duration_count == 0) { config.focus_durations[0] = 30; config.focus_duration_count = 1; }
        if (config.focus_duration_index >= config.focus_duration_count) config.focus_duration_index = 0;
    }
    if (doc.containsKey("focus_flash")) config.focus_flash = doc["focus_flash"] == 1;
    
    if (doc.containsKey("hide_empty_pc")) config.hide_empty_pc = doc["hide_empty_pc"] == 1;
    if (doc.containsKey("hide_empty_media")) config.hide_empty_media = doc["hide_empty_media"] == 1;
    if (doc.containsKey("hide_empty_bambu")) config.hide_empty_bambu = doc["hide_empty_bambu"] == 1;

    if (doc.containsKey("screen_order")) {
        String orderStr = doc["screen_order"].as<String>();
        int idx = 0; int startPos = 0;
        while (startPos < orderStr.length() && idx < NUM_SCREENS) {
            int commaPos = orderStr.indexOf(',', startPos);
            if (commaPos == -1) {
                config.screen_order[idx++] = orderStr.substring(startPos).toInt(); break;
            } else {
                config.screen_order[idx++] = orderStr.substring(startPos, commaPos).toInt(); startPos = commaPos + 1;
            }
        }
    }

    // Dynamic State Wipes (Triggers data reload)
    state.calendar.last_fetch_year = -1;
    state.calendar.count = 0;

    if (!config.show_weather) { state.weather.temp = NAN; state.weather.humidity = NAN; state.weather.apparent_temperature = NAN; state.weather.wind_speed = NAN; }
    if (!config.show_aqi) { state.aqi.aqi = NAN; state.aqi.pm25 = NAN; state.aqi.pm10 = NAN; state.aqi.no2 = NAN; }
    if (!config.show_daylight) { state.daylight.sunrise_mins = -1; state.daylight.sunset_mins = -1; state.daylight.noon_mins = -1; state.daylight.length_mins = -1; state.daylight.last_fetch_yday = -1; }
    if (!config.show_pc) { state.pc.cpu_percent = 0; state.pc.net_down_kb = 0; state.pc.mem_percent = 0; state.pc.disk_percent = 0; }
    if (!config.show_focus) { state.focus.state = FOCUS_STATE_IDLE; state.focus.elapsed_ms = 0; state.focus.segment_start_ms = 0; }
    
    // Array Wipes
    if (!config.show_crypto) { 
        for(int i=0; i<MAX_MULTI_ENTRIES; i++) { state.cryptos[i].price_usd = NAN; state.cryptos[i].percent_change_24h = NAN; state.cryptos[i].updated = false; } 
    }
    if (!config.show_currency) { 
        for(int i=0; i<MAX_MULTI_ENTRIES; i++) { state.currencies[i].rate = NAN; state.currencies[i].updated = false; } 
    }
    if (!config.show_stock) { 
        for(int i=0; i<MAX_MULTI_ENTRIES; i++) { state.stocks[i].price = NAN; state.stocks[i].percent_change = NAN; state.stocks[i].updated = false; } 
    }
    
    if (!config.show_media) { state.media.status = "stopped"; state.media.name = ""; }

    return true;
}