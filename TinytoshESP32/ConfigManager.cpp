#include "ConfigManager.h"
#include <Arduino.h>

ConfigManager::ConfigManager(const char* ns) : PREF_NAMESPACE(ns) {}

void ConfigManager::loadConfig(Config& config) {
  preferences.begin(PREF_NAMESPACE, true);

  // Global Settings
  config.auto_detect = preferences.getBool("auto_detect", true);
  config.latitude = preferences.getFloat("latitude", 0.0);
  config.longitude = preferences.getFloat("longitude", 0.0);
  config.country = preferences.getString("country", "");
  config.country_code = preferences.getString("country_c", "");
  config.city = preferences.getString("city", "");
  config.timezone = preferences.getString("timezone", "");
  config.time_format = preferences.getString("time_format", "24");
  config.date_display = preferences.getBool("date_display", true);
  config.refresh_interval_min = preferences.getULong("refresh_min", 15);

  // Screens Settings
  config.screen_auto_cycle = preferences.getBool("auto_cycle", true);
  config.screen_interval_sec = preferences.getInt("scr_int", 15);

  // Custom Screen Order
  size_t orderLen = preferences.getBytesLength("scr_order");
  if (orderLen == sizeof(config.screen_order)) {
    preferences.getBytes("scr_order", config.screen_order, sizeof(config.screen_order));
  }

  config.show_time = preferences.getBool("show_time", true);
  config.show_calendar = preferences.getBool("show_calendar", true);
  config.show_weather = preferences.getBool("show_weather", true);
  config.show_aqi = preferences.getBool("show_aqi", true);
  // show_stocks and stock_symbols are loaded below in the stock settings block
  config.show_crypto = preferences.getBool("show_crypto", true);
  config.show_currency = preferences.getBool("show_curr", true);
  config.show_pc = preferences.getBool("show_pc", true);
  config.show_media = preferences.getBool("show_media", true);
  config.show_bambu = preferences.getBool("show_bambu", true);

  config.hide_empty_pc = preferences.getBool("hide_pc", true);
  config.hide_empty_media = preferences.getBool("hide_media", true);
  config.hide_empty_bambu = preferences.getBool("hide_bambu", true);

  // Calendar Settings
  config.calendar_start_day = preferences.getString("cal_start", "mon");
  config.calendar_show_holidays = preferences.getBool("cal_hol", true);
  config.calendar_minimal = preferences.getBool("cal_min", false);

  // Weather & AQI Settings
  config.round_temps = preferences.getBool("round_temps", true);
  config.temp_unit = preferences.getString("temp_unit", "C");
  config.aqi_type = preferences.getString("aqi_type", "EU");

  // Crypto, Currency & Stock Settings
  config.crypto_id = preferences.getInt("crypto_id", 90);
  config.currency_base = preferences.getString("cur_base", "usd");
  config.currency_target = preferences.getString("cur_targ", "eur");
  config.currency_multiplier = preferences.getInt("cur_m", 1);
  const char* stockDefaults[5] = {"GOOG", "", "", "", ""};
  for (int i = 0; i < 5; i++) {
    String symKey = "stk_sym_" + String(i);
    String showKey = "show_stk_" + String(i);
    config.stock_symbols[i] = preferences.getString(symKey.c_str(), stockDefaults[i]);
    config.show_stocks[i] = preferences.getBool(showKey.c_str(), i == 0);
  }
  config.finnhub_key = preferences.getString("finnhub_key", "");
  config.crypto_fn = preferences.getBool("crypto_fn", true);
  config.currency_fn = preferences.getBool("cur_fn", true);
  config.stock_fn = preferences.getBool("stock_fn", true);

  // Animation Settings
  config.anim_mask = preferences.getUShort("anim_mask", 62);

  // Night Mode Settings
  config.night_mode = preferences.getBool("night_mode", false);
  config.night_start = preferences.getString("night_start", "23:00");
  config.night_end = preferences.getString("night_end", "06:00");
  config.night_dim_start = preferences.getString("night_dim", "22:00");
  config.night_action = preferences.getInt("night_action", 1);

  // Printer Settings
  config.bambu_ip = preferences.getString("bambu_ip", "");
  config.bambu_sn = preferences.getString("bambu_sn", "");
  config.bambu_code = preferences.getString("bambu_code", "");

  preferences.end();
  Serial.println("ConfigManager: Configuration loaded from NVS.");
}

void ConfigManager::saveConfig(const Config& config) {
  Serial.println("ConfigManager: Saving config to NVS...");
  preferences.begin(PREF_NAMESPACE, false);

  // Global Settings
  preferences.putBool("auto_detect", config.auto_detect);
  preferences.putFloat("latitude", config.latitude);
  preferences.putFloat("longitude", config.longitude);
  preferences.putString("country", config.country);
  preferences.putString("country_c", config.country_code);
  preferences.putString("city", config.city);
  preferences.putString("timezone", config.timezone);
  preferences.putString("time_format", config.time_format);
  preferences.putBool("date_display", config.date_display);
  preferences.putULong("refresh_min", config.refresh_interval_min);

  // Screens Settings
  preferences.putBool("auto_cycle", config.screen_auto_cycle);
  preferences.putInt("scr_int", config.screen_interval_sec);
  preferences.putBytes("scr_order", config.screen_order, sizeof(config.screen_order));

  preferences.putBool("show_time", config.show_time);
  preferences.putBool("show_calendar", config.show_calendar);
  preferences.putBool("show_weather", config.show_weather);
  preferences.putBool("show_aqi", config.show_aqi);
  // show_stocks saved below in stock settings block
  preferences.putBool("show_crypto", config.show_crypto);
  preferences.putBool("show_curr", config.show_currency);
  preferences.putBool("show_pc", config.show_pc);
  preferences.putBool("show_media", config.show_media);
  preferences.putBool("show_bambu", config.show_bambu);

  preferences.putBool("hide_pc", config.hide_empty_pc);
  preferences.putBool("hide_media", config.hide_empty_media);
  preferences.putBool("hide_bambu", config.hide_empty_bambu);

  // Calendar Settings
  preferences.putString("cal_start", config.calendar_start_day);
  preferences.putBool("cal_hol", config.calendar_show_holidays);
  preferences.putBool("cal_min", config.calendar_minimal);

  // Weather & AQI Settings
  preferences.putBool("round_temps", config.round_temps);
  preferences.putString("temp_unit", config.temp_unit);
  preferences.putString("aqi_type", config.aqi_type);

  // Crypto, Currency & Stock Settings
  preferences.putInt("crypto_id", config.crypto_id);
  preferences.putString("cur_base", config.currency_base);
  preferences.putString("cur_targ", config.currency_target);
  preferences.putInt("cur_m", config.currency_multiplier);
  for (int i = 0; i < 5; i++) {
    String symKey = "stk_sym_" + String(i);
    String showKey = "show_stk_" + String(i);
    preferences.putString(symKey.c_str(), config.stock_symbols[i]);
    preferences.putBool(showKey.c_str(), config.show_stocks[i]);
  }
  preferences.putString("finnhub_key", config.finnhub_key);
  preferences.putBool("crypto_fn", config.crypto_fn);
  preferences.putBool("cur_fn", config.currency_fn);
  preferences.putBool("stock_fn", config.stock_fn);

  // Animation Settings
  preferences.putUShort("anim_mask", config.anim_mask);

  // Night Mode Settings
  preferences.putBool("night_mode", config.night_mode);
  preferences.putString("night_start", config.night_start);
  preferences.putString("night_end", config.night_end);
  preferences.putString("night_dim", config.night_dim_start);
  preferences.putInt("night_action", config.night_action);

  // Printer Settings
  preferences.putString("bambu_ip", config.bambu_ip);
  preferences.putString("bambu_sn", config.bambu_sn);
  preferences.putString("bambu_code", config.bambu_code);

  preferences.end();
  Serial.println("ConfigManager: Config saved.");
}

void ConfigManager::clearAllPreferences() {
  preferences.begin(PREF_NAMESPACE, false);
  preferences.clear();
  preferences.end();
  Serial.println("Preferences cleared!");
}