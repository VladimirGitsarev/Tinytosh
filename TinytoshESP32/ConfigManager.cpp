#include "ConfigManager.h"
#include <Arduino.h>

ConfigManager::ConfigManager(const char* ns) : PREF_NAMESPACE(ns) {}

void ConfigManager::loadConfig(Config& config) {
  preferences.begin(PREF_NAMESPACE, true); 

  config.auto_detect = preferences.getBool("auto_detect", true);
  config.latitude = preferences.getFloat("latitude", 0.0);
  config.longitude = preferences.getFloat("longitude", 0.0);
  config.timezone = preferences.getString("timezone", ""); 
  config.city = preferences.getString("city", "");
  config.time_format = preferences.getString("time_format", "24");
  config.date_display = preferences.getBool("date_display", true);
  config.refresh_interval_min = preferences.getULong("refresh_min", 15); 
  config.screen_auto_cycle = preferences.getBool("auto_cycle", true);
  config.anim_mask = preferences.getUShort("anim_mask", 62);
  config.round_temps = preferences.getBool("round_temps", true);
  config.temp_unit = preferences.getString("temp_unit", "C"); 
  config.aqi_type = preferences.getString("aqi_type", "EU");
  config.crypto_id = preferences.getInt("crypto_id", 90);
  config.screen_interval_sec = preferences.getInt("scr_int", 15);

  config.show_time = preferences.getBool("show_time", true);
  config.show_weather = preferences.getBool("show_weather", true);
  config.show_aqi = preferences.getBool("show_aqi", true);
  config.show_crypto = preferences.getBool("show_crypto", true);
  config.show_pc = preferences.getBool("show_pc", true);

  preferences.end();
  Serial.println("ConfigManager: Configuration loaded from NVS."); 
}

void ConfigManager::saveConfig(const Config& config) {
  Serial.println("ConfigManager: Saving config to NVS..."); 
  preferences.begin(PREF_NAMESPACE, false); 

  preferences.putBool("auto_detect", config.auto_detect);
  preferences.putFloat("latitude", config.latitude);
  preferences.putFloat("longitude", config.longitude);
  preferences.putString("timezone", config.timezone);
  preferences.putString("city", config.city);
  preferences.putString("time_format", config.time_format);
  preferences.putBool("date_display", config.date_display);
  preferences.putULong("refresh_min", config.refresh_interval_min);
  preferences.putBool("auto_cycle", config.screen_auto_cycle);
  preferences.putUShort("anim_mask", config.anim_mask);
  preferences.putBool("round_temps", config.round_temps);
  preferences.putString("temp_unit", config.temp_unit);
  preferences.putString("aqi_type", config.aqi_type);
  preferences.putInt("crypto_id", config.crypto_id);
  preferences.putInt("scr_int", config.screen_interval_sec);

  preferences.putBool("show_time", config.show_time);
  preferences.putBool("show_weather", config.show_weather);
  preferences.putBool("show_aqi", config.show_aqi);
  preferences.putBool("show_pc", config.show_pc);
  preferences.putBool("show_crypto", config.show_crypto);

  preferences.end();
  Serial.println("ConfigManager: Config saved."); 
}

void ConfigManager::clearAllPreferences() {
    preferences.begin(PREF_NAMESPACE, false); 
    preferences.clear(); 
    preferences.end();
    Serial.println("Preferences cleared!");
}