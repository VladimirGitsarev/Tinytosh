#include "JsonSerializer.h"

#include "PopulationService.h"
#include "TimeService.h"

void JsonSerializer::populateConfigDoc(const Config& config, JsonObject configObj) {
    JsonObject network = configObj.createNestedObject("network");
    network["device_id"] = config.device_id;
    network["ip_address"] = config.ip_address;

    JsonObject hardware = configObj.createNestedObject("hardware");
    hardware["sda_pin"] = config.sda_pin;
    hardware["scl_pin"] = config.scl_pin;
    hardware["button_pin"] = config.button_pin;
    hardware["button_type"] = config.button_type;

    JsonObject general = configObj.createNestedObject("general");
    general["refresh_min"] = config.refresh_interval_min;
    general["time_format"] = config.time_format;
    general["auto_detect"] = config.auto_detect ? 1 : 0;
    general["latitude"] = config.latitude;
    general["longitude"] = config.longitude;
    general["country"] = config.country;
    general["country_code"] = config.country_code;
    general["city"] = config.city;
    general["timezone"] = config.timezone;
    general["date_display"] = config.date_display ? 1 : 0;

    JsonObject theme = configObj.createNestedObject("theme");
    theme["bg"] = config.theme_bg;
    theme["card"] = config.theme_card;
    theme["accent"] = config.theme_accent;
    theme["text"] = config.theme_text;

    JsonObject night = configObj.createNestedObject("night");
    night["mode"] = config.night_mode ? 1 : 0;
    night["start"] = config.night_start;
    night["end"] = config.night_end;
    night["action"] = config.night_action;
    night["dim_start"] = config.night_dim_start;

    JsonObject screens = configObj.createNestedObject("screens");
    screens["auto_cycle"] = config.screen_auto_cycle ? 1 : 0;
    screens["interval_sec"] = config.screen_interval_sec;
    screens["anim_mask"] = config.anim_mask;
    screens["show_time"] = config.show_time ? 1 : 0;
    screens["show_calendar"] = config.show_calendar ? 1 : 0;
    screens["show_weather"] = config.show_weather ? 1 : 0;
    screens["show_aqi"] = config.show_aqi ? 1 : 0;
    screens["show_daylight"] = config.show_daylight ? 1 : 0;
    screens["show_moon"] = config.show_moon ? 1 : 0;
    screens["show_population"] = config.show_population ? 1 : 0;
    screens["show_pc"] = config.show_pc ? 1 : 0;
    screens["show_media"] = config.show_media ? 1 : 0;
    screens["show_stock"] = config.show_stock ? 1 : 0;
    screens["show_crypto"] = config.show_crypto ? 1 : 0;
    screens["show_currency"] = config.show_currency ? 1 : 0;
    screens["show_bambu"] = config.show_bambu ? 1 : 0;
    screens["show_flight"] = config.show_flight ? 1 : 0;
    screens["hide_empty_pc"] = config.hide_empty_pc ? 1 : 0;
    screens["hide_empty_media"] = config.hide_empty_media ? 1 : 0;
    screens["hide_empty_bambu"] = config.hide_empty_bambu ? 1 : 0;
    screens["hide_empty_flight"] = config.hide_empty_flight ? 1 : 0;
    String orderStr = "";
    for (int i = 0; i < NUM_SCREENS; i++) {
        orderStr += String(config.screen_order[i]);
        if (i < NUM_SCREENS - 1) orderStr += ",";
    }
    screens["order"] = orderStr;

    JsonObject calendar = configObj.createNestedObject("calendar");
    calendar["start_day"] = config.calendar_start_day;
    calendar["show_holidays"] = config.calendar_show_holidays ? 1 : 0;
    calendar["minimal"] = config.calendar_minimal ? 1 : 0;

    JsonObject weather = configObj.createNestedObject("weather");
    weather["temp_unit"] = config.temp_unit;
    weather["round_temps"] = config.round_temps ? 1 : 0;
    weather["show_header"] = config.weather_show_header ? 1 : 0;
    weather["custom_sync_min"] = config.custom_weather_int_min;
    JsonArray wvArr = weather.createNestedArray("values");
    for (int i = 0; i < 6; i++) if (config.weather_values[i].length() > 0) wvArr.add(config.weather_values[i]);

    JsonObject aqi = configObj.createNestedObject("aqi");
    aqi["type"] = config.aqi_type;
    aqi["show_header"] = config.aqi_show_header ? 1 : 0;
    aqi["custom_sync_min"] = config.custom_aqi_int_min;
    JsonArray avArr = aqi.createNestedArray("values");
    for (int i = 0; i < 6; i++) if (config.aqi_values[i].length() > 0) avArr.add(config.aqi_values[i]);

    JsonObject daylight = configObj.createNestedObject("daylight");
    daylight["minimal"] = config.daylight_minimal ? 1 : 0;

    JsonObject moon = configObj.createNestedObject("moon");
    moon["minimal"] = config.moon_minimal ? 1 : 0;

    JsonObject population = configObj.createNestedObject("population");
    population["show_world"] = config.pop_show_world ? 1 : 0;
    population["show_country"] = config.pop_show_country ? 1 : 0;

    JsonObject stocks = configObj.createNestedObject("stocks");
    stocks["fn"] = config.stock_fn ? 1 : 0;
    stocks["custom_sync_min"] = config.custom_stock_int_min;
    JsonArray stArr = stocks.createNestedArray("symbols");
    for (int i = 0; i < config.stock_count; i++) stArr.add(config.stock_symbols[i]);

    JsonObject crypto = configObj.createNestedObject("crypto");
    crypto["fn"] = config.crypto_fn ? 1 : 0;
    crypto["custom_sync_min"] = config.custom_crypto_int_min;
    JsonArray crArr = crypto.createNestedArray("ids");
    for (int i = 0; i < config.crypto_count; i++) crArr.add(config.crypto_ids[i]);

    JsonObject currency = configObj.createNestedObject("currency");
    currency["fn"] = config.currency_fn ? 1 : 0;
    currency["custom_sync_min"] = config.custom_currency_int_min;
    JsonArray cbArr = currency.createNestedArray("bases");
    for (int i = 0; i < config.currency_count; i++) cbArr.add(config.currency_bases[i]);
    JsonArray ctArr = currency.createNestedArray("targets");
    for (int i = 0; i < config.currency_count; i++) ctArr.add(config.currency_targets[i]);
    JsonArray cmArr = currency.createNestedArray("multipliers");
    for (int i = 0; i < config.currency_count; i++) cmArr.add(config.currency_multipliers[i]);

    JsonObject printer = configObj.createNestedObject("printer");
    printer["ip"] = config.bambu_ip;
    printer["sn"] = config.bambu_sn;
    printer["code"] = config.bambu_code;

    JsonObject flight = configObj.createNestedObject("flight");
    flight["mode"] = config.flight_mode;
    flight["radius_nm"] = config.flight_radius_nm;
    flight["units"] = config.flight_units;
    flight["primary_info"] = config.flight_primary_info;
    flight["secondary_info"] = config.flight_secondary_info;
    flight["custom_sync_min"] = config.custom_flight_int_min;
}

String JsonSerializer::buildConfigJson(const Config& config) {
    DynamicJsonDocument doc(3072);
    JsonObject root = doc.to<JsonObject>();
    populateConfigDoc(config, root);
    String output;
    serializeJson(doc, output);
    return output;
}

String JsonSerializer::buildAppStateJson(const AppState& state) {
    DynamicJsonDocument doc(6144);

    // 1. Config, grouped identically to buildConfigJson's output
    JsonObject configObj = doc.createNestedObject("config");
    populateConfigDoc(state.config, configObj);

    // 2. Live status, grouped by the same domains as config
    JsonObject status = doc.createNestedObject("status");

    JsonObject general = status.createNestedObject("general");
    general["time"] = TimeService::getCurrentTimeShort(state.config.time_format);
    general["date"] = TimeService::getFullDate();

    JsonObject calendar = status.createNestedObject("calendar");
    calendar["count"] = state.calendar.count;

    JsonObject weather = status.createNestedObject("weather");
    weather["update_time"] = state.weather.update_time;
    if (!isnan(state.weather.temp)) {
        weather["temp"] = String(state.weather.temp, 1);
        weather["apparent_temperature"] = String(state.weather.apparent_temperature, 1);
        weather["humidity"] = String(state.weather.humidity);
        weather["wind_speed"] = String(state.weather.wind_speed, 1);
        weather["weather_code"] = state.weather.weather_code;
        weather["precipitation_probability"] = String(state.weather.precipitation_probability, 0);
        weather["pressure"] = String(state.weather.pressure, 0);
        weather["visibility"] = String(state.weather.visibility / 1000.0, 1);
    }

    if (state.aqi.aqi != -1) {
        JsonObject aqi = status.createNestedObject("aqi");
        aqi["index"] = String(state.aqi.aqi);
        aqi["status"] = state.aqi.status;
        aqi["pm25"] = String(state.aqi.pm25, 1);
        aqi["pm10"] = String(state.aqi.pm10, 1);
        aqi["no2"] = String(state.aqi.no2, 1);
        aqi["co"] = String(state.aqi.co, 1);
        aqi["co2"] = String(state.aqi.co2, 1);
        aqi["so2"] = String(state.aqi.so2, 1);
        aqi["o3"] = String(state.aqi.o3, 1);
        aqi["dust"] = String(state.aqi.dust, 1);
        aqi["uv"] = String(state.aqi.uv, 1);
        aqi["ch4"] = String(state.aqi.ch4, 1);
    }

    if (state.daylight.sunrise_mins != -1) {
        JsonObject daylight = status.createNestedObject("daylight");
        daylight["sunrise"] = TimeService::formatMinsFromMidnight(state.daylight.sunrise_mins, state.config.time_format);
        daylight["sunset"] = TimeService::formatMinsFromMidnight(state.daylight.sunset_mins, state.config.time_format);
        daylight["solar_noon"] = TimeService::formatMinsFromMidnight(state.daylight.noon_mins, state.config.time_format);
        daylight["day_length"] = TimeService::formatDurationMins(state.daylight.length_mins);
    }

    if (state.moon.curphase != "N/A") {
        JsonObject moon = status.createNestedObject("moon");
        moon["rise"] = state.moon.rise_mins != -1 ? TimeService::formatMinsFromMidnight(state.moon.rise_mins, state.config.time_format) : "--:--";
        moon["set"] = state.moon.set_mins != -1 ? TimeService::formatMinsFromMidnight(state.moon.set_mins, state.config.time_format) : "--:--";
        moon["phase"] = state.moon.curphase;
        moon["illum"] = state.moon.fracillum;
    }

    if ((state.config.pop_show_world && state.population.world_pop_base != -1) ||
        (state.config.pop_show_country && state.population.country_pop_base != -1)) {
        JsonObject population = status.createNestedObject("population");
        if (state.config.pop_show_world && state.population.world_pop_base != -1) {
            population["world_live"] = String(PopulationService::getLivePopulation(state.population.world_pop_base, state.population.world_growth, state.population.world_year));
            population["world_growth"] = String(state.population.world_growth, 2);
        }
        if (state.config.pop_show_country && state.population.country_pop_base != -1) {
            population["country_live"] = String(PopulationService::getLivePopulation(state.population.country_pop_base, state.population.country_growth, state.population.country_year));
            population["country_growth"] = String(state.population.country_growth, 2);
        }
    }

    JsonObject stocks = status.createNestedObject("stocks");
    JsonArray stockData = stocks.createNestedArray("data");
    for (int i = 0; i < state.config.stock_count; i++) {
        if (state.stocks[i].updated) {
            JsonObject obj = stockData.createNestedObject();
            obj["symbol"] = state.stocks[i].symbol;
            obj["price"] = String(state.stocks[i].price, 2);
            obj["change"] = String(state.stocks[i].percent_change, 2);
        }
    }

    JsonObject crypto = status.createNestedObject("crypto");
    JsonArray cryptoData = crypto.createNestedArray("data");
    for (int i = 0; i < state.config.crypto_count; i++) {
        if (state.cryptos[i].updated) {
            JsonObject obj = cryptoData.createNestedObject();
            obj["symbol"] = String(state.cryptos[i].symbol);
            obj["price"] = String(state.cryptos[i].price_usd);
            obj["change"] = String(state.cryptos[i].percent_change_24h, 1);
        }
    }

    JsonObject currency = status.createNestedObject("currency");
    JsonArray currencyData = currency.createNestedArray("data");
    for (int i = 0; i < state.config.currency_count; i++) {
        if (state.currencies[i].updated) {
            JsonObject obj = currencyData.createNestedObject();
            float displayRate = state.currencies[i].rate * state.config.currency_multipliers[i];
            int decimals = (displayRate < 10.0) ? 3 : (displayRate < 100.0) ? 2 : (displayRate < 1000.0) ? 1 : 0;
            obj["base_text"] = String(state.config.currency_multipliers[i]) + " " + state.currencies[i].base;
            obj["target_text"] = String(displayRate, decimals) + " " + state.currencies[i].target;
        }
    }

    JsonObject pc = status.createNestedObject("pc");
    if (state.pc.cpu_percent > 0.1) {
        pc["cpu"] = String(state.pc.cpu_percent);
        pc["net"] = String(state.pc.net_down_kb);
        pc["ram"] = String(state.pc.mem_percent);
        pc["disk"] = String(state.pc.disk_percent);
    }
    String activeId = state.config.active_pc_id;
    int lastColonPos = activeId.lastIndexOf(':');
    if (lastColonPos > 3) activeId = activeId.substring(0, lastColonPos);
    bool isPcPaired = (activeId != "" && (millis() - state.pc.last_update < 10000));
    pc["status"] = isPcPaired ? ("🔒 Paired to " + activeId) : "";

    if (state.media.status.length() > 0) {
        JsonObject media = status.createNestedObject("media");
        media["status"] = state.media.status;
        media["name"] = state.media.name;
        media["author"] = state.media.author;
        media["album"] = state.media.album;
    }

    if (state.bambu.status != "SYNCING") {
        JsonObject printer = status.createNestedObject("printer");
        printer["status"] = state.bambu.status;
        printer["progress"] = state.bambu.progress;
        printer["time"] = state.bambu.time_left;
        printer["layer"] = state.bambu.layer;
        printer["total_layers"] = state.bambu.total_layers;
        printer["nozzle"] = String(state.bambu.nozzle_temp, 1);
        printer["nozzle_target"] = String(state.bambu.nozzle_target, 1);
        printer["bed"] = String(state.bambu.bed_temp, 1);
        printer["bed_target"] = String(state.bambu.bed_target, 1);
        printer["fan_part"] = state.bambu.fan_part;
        printer["fan_aux"] = state.bambu.fan_aux;
    }

    bool flightValid = (state.config.flight_mode == "closest") ? (state.flight.closest.callsign.length() > 0) : (state.flight.aircraft_count > 0);
    if (flightValid) {
        const FlightAircraft& c = (state.config.flight_mode == "closest") ? state.flight.closest : state.flight.aircraft[0];
        JsonObject flight = status.createNestedObject("flight");
        flight["count"] = state.flight.aircraft_count;
        flight["callsign"] = c.callsign;
        if (state.config.flight_mode == "closest") {
            flight["route"] = c.has_route ? (c.origin_code + " &rarr; " + c.destination_code) : "N/A";
        }
    }

    String output;
    serializeJson(doc, output);
    return output;
}

bool JsonSerializer::parseConfig(const char* jsonString, AppState& state) {
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) return false;

    Config& config = state.config;

    if (doc.containsKey("hardware")) {
        JsonObject g = doc["hardware"];
        if (g.containsKey("sda_pin")) config.sda_pin = g["sda_pin"];
        if (g.containsKey("scl_pin")) config.scl_pin = g["scl_pin"];
        if (g.containsKey("button_pin")) config.button_pin = g["button_pin"];
        if (g.containsKey("button_type")) config.button_type = g["button_type"].as<String>();
    }

    if (doc.containsKey("general")) {
        JsonObject g = doc["general"];
        if (g.containsKey("refresh_min")) config.refresh_interval_min = g["refresh_min"];
        if (g.containsKey("time_format")) config.time_format = g["time_format"].as<String>();
        if (g.containsKey("auto_detect")) config.auto_detect = g["auto_detect"] == 1;
        if (g.containsKey("latitude")) config.latitude = g["latitude"].as<float>();
        if (g.containsKey("longitude")) config.longitude = g["longitude"].as<float>();
        if (g.containsKey("country")) config.country = g["country"].as<String>();
        if (g.containsKey("city")) config.city = g["city"].as<String>();
        if (g.containsKey("timezone")) config.timezone = g["timezone"].as<String>();
        if (g.containsKey("date_display")) config.date_display = g["date_display"] == 1;
        if (g.containsKey("country_code")) {
            config.country_code = g["country_code"].as<String>();
            for (auto c : allCountries) {
                if (config.country_code == String(c.code)) {
                    config.country = c.name;
                    break;
                }
            }
        }
    }

    if (doc.containsKey("theme")) {
        JsonObject g = doc["theme"];
        if (g.containsKey("bg")) config.theme_bg = g["bg"].as<String>();
        if (g.containsKey("card")) config.theme_card = g["card"].as<String>();
        if (g.containsKey("accent")) config.theme_accent = g["accent"].as<String>();
        if (g.containsKey("text")) config.theme_text = g["text"].as<String>();
    }

    if (doc.containsKey("night")) {
        JsonObject g = doc["night"];
        if (g.containsKey("mode")) config.night_mode = g["mode"] == 1;
        if (g.containsKey("start")) config.night_start = g["start"].as<String>();
        if (g.containsKey("end")) config.night_end = g["end"].as<String>();
        if (g.containsKey("dim_start")) config.night_dim_start = g["dim_start"].as<String>();
        if (g.containsKey("action")) config.night_action = g["action"];
    }

    if (doc.containsKey("screens")) {
        JsonObject g = doc["screens"];
        if (g.containsKey("auto_cycle")) config.screen_auto_cycle = g["auto_cycle"] == 1;
        if (g.containsKey("interval_sec")) config.screen_interval_sec = g["interval_sec"];
        if (g.containsKey("anim_mask")) config.anim_mask = g["anim_mask"];
        if (g.containsKey("show_time")) config.show_time = g["show_time"] == 1;
        if (g.containsKey("show_calendar")) config.show_calendar = g["show_calendar"] == 1;
        if (g.containsKey("show_weather")) config.show_weather = g["show_weather"] == 1;
        if (g.containsKey("show_aqi")) config.show_aqi = g["show_aqi"] == 1;
        if (g.containsKey("show_daylight")) config.show_daylight = g["show_daylight"] == 1;
        if (g.containsKey("show_moon")) config.show_moon = g["show_moon"] == 1;
        if (g.containsKey("show_population")) config.show_population = g["show_population"] == 1;
        if (g.containsKey("show_pc")) config.show_pc = g["show_pc"] == 1;
        if (g.containsKey("show_media")) config.show_media = g["show_media"] == 1;
        if (g.containsKey("show_stock")) config.show_stock = g["show_stock"] == 1;
        if (g.containsKey("show_crypto")) config.show_crypto = g["show_crypto"] == 1;
        if (g.containsKey("show_currency")) config.show_currency = g["show_currency"] == 1;
        if (g.containsKey("show_bambu")) config.show_bambu = g["show_bambu"] == 1;
        if (g.containsKey("show_flight")) config.show_flight = g["show_flight"] == 1;
        if (g.containsKey("hide_empty_pc")) config.hide_empty_pc = g["hide_empty_pc"] == 1;
        if (g.containsKey("hide_empty_media")) config.hide_empty_media = g["hide_empty_media"] == 1;
        if (g.containsKey("hide_empty_bambu")) config.hide_empty_bambu = g["hide_empty_bambu"] == 1;
        if (g.containsKey("hide_empty_flight")) config.hide_empty_flight = g["hide_empty_flight"] == 1;
        if (g.containsKey("order")) {
            String orderStr = g["order"].as<String>();
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
    }

    if (doc.containsKey("calendar")) {
        JsonObject g = doc["calendar"];
        if (g.containsKey("start_day")) config.calendar_start_day = g["start_day"].as<String>();
        if (g.containsKey("show_holidays")) config.calendar_show_holidays = g["show_holidays"] == 1;
        if (g.containsKey("minimal")) config.calendar_minimal = g["minimal"] == 1;
    }

    if (doc.containsKey("weather")) {
        JsonObject g = doc["weather"];
        if (g.containsKey("temp_unit")) config.temp_unit = g["temp_unit"].as<String>();
        if (g.containsKey("round_temps")) config.round_temps = g["round_temps"] == 1;
        if (g.containsKey("show_header")) config.weather_show_header = g["show_header"] == 1;
        if (g.containsKey("custom_sync_min")) config.custom_weather_int_min = g["custom_sync_min"];
        if (g.containsKey("values")) {
            JsonArray arr = g["values"].as<JsonArray>();
            for (int i = 0; i < 6; i++) config.weather_values[i] = "";
            int i = 0;
            for (JsonVariant v : arr) if (i < 6) config.weather_values[i++] = v.as<String>();
        }
    }

    if (doc.containsKey("aqi")) {
        JsonObject g = doc["aqi"];
        if (g.containsKey("type")) config.aqi_type = g["type"].as<String>();
        if (g.containsKey("show_header")) config.aqi_show_header = g["show_header"] == 1;
        if (g.containsKey("custom_sync_min")) config.custom_aqi_int_min = g["custom_sync_min"];
        if (g.containsKey("values")) {
            JsonArray arr = g["values"].as<JsonArray>();
            for (int i = 0; i < 6; i++) config.aqi_values[i] = "";
            int i = 0;
            for (JsonVariant v : arr) if (i < 6) config.aqi_values[i++] = v.as<String>();
        }
    }

    if (doc.containsKey("daylight")) {
        JsonObject g = doc["daylight"];
        if (g.containsKey("minimal")) config.daylight_minimal = g["minimal"] == 1;
    }

    if (doc.containsKey("moon")) {
        JsonObject g = doc["moon"];
        if (g.containsKey("minimal")) config.moon_minimal = g["minimal"] == 1;
    }

    if (doc.containsKey("population")) {
        JsonObject g = doc["population"];
        if (g.containsKey("show_world")) config.pop_show_world = g["show_world"] == 1;
        if (g.containsKey("show_country")) config.pop_show_country = g["show_country"] == 1;
    }

    if (doc.containsKey("stocks")) {
        JsonObject g = doc["stocks"];
        if (g.containsKey("fn")) config.stock_fn = g["fn"] == 1;
        if (g.containsKey("custom_sync_min")) config.custom_stock_int_min = g["custom_sync_min"];
        if (g.containsKey("symbols")) {
            JsonArray arr = g["symbols"].as<JsonArray>();
            config.stock_count = 0;
            for (JsonVariant v : arr) if (config.stock_count < MAX_MULTI_ENTRIES) config.stock_symbols[config.stock_count++] = v.as<String>();
            if (config.stock_count == 0) { config.stock_symbols[0] = "AAPL"; config.stock_count = 1; }
        }
    }

    if (doc.containsKey("crypto")) {
        JsonObject g = doc["crypto"];
        if (g.containsKey("fn")) config.crypto_fn = g["fn"] == 1;
        if (g.containsKey("custom_sync_min")) config.custom_crypto_int_min = g["custom_sync_min"];
        if (g.containsKey("ids")) {
            JsonArray arr = g["ids"].as<JsonArray>();
            config.crypto_count = 0;
            for (JsonVariant v : arr) if (config.crypto_count < MAX_MULTI_ENTRIES) config.crypto_ids[config.crypto_count++] = v.as<int>();
            if (config.crypto_count == 0) { config.crypto_ids[0] = 90; config.crypto_count = 1; }
        }
    }

    if (doc.containsKey("currency")) {
        JsonObject g = doc["currency"];
        if (g.containsKey("fn")) config.currency_fn = g["fn"] == 1;
        if (g.containsKey("custom_sync_min")) config.custom_currency_int_min = g["custom_sync_min"];
        if (g.containsKey("bases")) {
            JsonArray arrB = g["bases"].as<JsonArray>();
            JsonArray arrT = g["targets"].as<JsonArray>();
            JsonArray arrM = g["multipliers"].as<JsonArray>();
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
    }

    if (doc.containsKey("printer")) {
        JsonObject g = doc["printer"];
        if (g.containsKey("ip")) config.bambu_ip = g["ip"].as<String>();
        if (g.containsKey("sn")) config.bambu_sn = g["sn"].as<String>();
        if (g.containsKey("code")) config.bambu_code = g["code"].as<String>();
    }

    if (doc.containsKey("flight")) {
        JsonObject g = doc["flight"];
        if (g.containsKey("mode")) config.flight_mode = g["mode"].as<String>();
        if (g.containsKey("radius_nm")) config.flight_radius_nm = g["radius_nm"];
        if (g.containsKey("units")) config.flight_units = g["units"].as<String>();
        if (g.containsKey("primary_info")) config.flight_primary_info = g["primary_info"].as<String>();
        if (g.containsKey("secondary_info")) config.flight_secondary_info = g["secondary_info"].as<String>();
        if (g.containsKey("custom_sync_min")) config.custom_flight_int_min = g["custom_sync_min"];
    }

    // Dynamic State Wipes (Triggers data reload)
    state.calendar.last_fetch_year = -1;
    state.calendar.count = 0;

    state.daylight.last_fetch_yday = -1;

    state.moon.last_fetch_yday = -1;
    state.population.last_fetch_yday = -1;

    if (!config.show_weather) { state.weather.temp = NAN; state.weather.humidity = NAN; state.weather.apparent_temperature = NAN; state.weather.wind_speed = NAN; }
    if (!config.show_aqi) { state.aqi.aqi = NAN; state.aqi.pm25 = NAN; state.aqi.pm10 = NAN; state.aqi.no2 = NAN; }
    if (!config.show_daylight) { state.daylight.sunrise_mins = -1; state.daylight.sunset_mins = -1; state.daylight.noon_mins = -1; state.daylight.length_mins = -1; state.daylight.last_fetch_yday = -1; }
    if (!config.show_moon) { state.moon.rise_mins = -1; state.moon.set_mins = -1; state.moon.curphase = "N/A"; state.moon.fracillum = -1; state.moon.last_fetch_yday = -1; }
    if (!config.show_population) { state.population.world_pop_base = -1; state.population.country_pop_base = -1; state.population.last_fetch_yday = -1; }
    if (!config.show_pc) { state.pc.cpu_percent = 0; state.pc.net_down_kb = 0; state.pc.mem_percent = 0; state.pc.disk_percent = 0; }
    if (!config.show_flight) { state.flight.closest = FlightAircraft(); state.flight.aircraft_count = 0; }

    // Array Wipes
    if (!config.show_crypto) {
        for (int i = 0; i < MAX_MULTI_ENTRIES; i++) { state.cryptos[i].price_usd = NAN; state.cryptos[i].percent_change_24h = NAN; state.cryptos[i].updated = false; }
    }
    if (!config.show_currency) {
        for (int i = 0; i < MAX_MULTI_ENTRIES; i++) { state.currencies[i].rate = NAN; state.currencies[i].updated = false; }
    }
    if (!config.show_stock) {
        for (int i = 0; i < MAX_MULTI_ENTRIES; i++) { state.stocks[i].price = NAN; state.stocks[i].percent_change = NAN; state.stocks[i].updated = false; }
    }

    if (!config.show_media) { state.media.status = "stopped"; state.media.name = ""; }

    return true;
}
