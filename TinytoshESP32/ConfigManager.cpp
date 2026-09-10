#include "ConfigManager.h"

#include <Arduino.h>

#include "JsonSerializer.h"

ConfigManager::ConfigManager(const char* ns) : PREF_NAMESPACE(ns) {}

void ConfigManager::loadConfig(Config& config) {
  preferences.begin(PREF_NAMESPACE, true);
  String json = preferences.getString("cfg_json", "");
  preferences.end();

  if (json.length() > 0) {
      AppState tempState;
      tempState.config = config;
      if (JsonSerializer::parseConfig(json.c_str(), tempState)) {
          config = tempState.config;
          Serial.println("ConfigManager: Configuration loaded from NVS JSON.");
      }
  } else {
      Serial.println("ConfigManager: No JSON found. Legacy upgrade or fresh install detected.");

      clearAllPreferences();
      
      saveConfig(config);
      
      Serial.println("ConfigManager: Memory wiped and initialized with fresh JSON defaults.");
  }
}

void ConfigManager::saveConfig(const Config& config) {
  Serial.println("ConfigManager: Saving config to NVS JSON...");
  String json = JsonSerializer::buildConfigJson(config);
  
  preferences.begin(PREF_NAMESPACE, false);
  preferences.putString("cfg_json", json);
  preferences.end();
  Serial.println("ConfigManager: Config saved.");
}

void ConfigManager::clearAllPreferences() {
  preferences.begin(PREF_NAMESPACE, false);
  preferences.clear();
  preferences.end();
  Serial.println("ConfigManager: Preferences completely cleared!");
}