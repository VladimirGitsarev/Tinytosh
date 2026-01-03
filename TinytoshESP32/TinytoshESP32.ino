#include <WiFi.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include "structs.h"
#include "ConfigManager.h"
#include "TimeService.h"
#include "WeatherService.h"
#include "AirQualityService.h"
#include "DisplayService.h"
#include "WebServerService.h"
#include "PcMonitorService.h"
#include "CryptoService.h"

// --- Global Constants ---
const char* AP_SSID = "Tinytosh";
const char* AP_PASS = "Tinytosh";
const char* PREF_NAMESPACE = "tinytosh_config";

// --- Global Data Structures ---
Config userConfig;
WeatherData weatherData;
AirQualityData airQualityData;
CryptoData cryptoData;
PcStats pcStats;

// Forward declaration of callback for WebServerService
void updateAllDataCallback();

// --- Service Instances ---
ConfigManager configManager(PREF_NAMESPACE);
TimeService timeService;
WeatherService weatherService;
AirQualityService airQualityService;
DisplayService displayService(128, 64, -1);
WebServerService webServerService(80, updateAllDataCallback);
PcMonitorService pcMonitorService;
CryptoService cryptoService;

unsigned long lastScreenSwitch = 0;
int currentScreen = SCREEN_TIME;

// --- Core Application Logic ---

void updateAllData() {
  // 1. Location Detection
  if (userConfig.auto_detect) {
      displayService.showOLEDStatus({"\n", "\n", "Detecting Location...", "\n", "Please wait..."}, true);
      if (timeService.fetchLocationData(userConfig)) {
          Serial.println("Location updated via IP");
      }
  }

  // 2. Sync Time (Depends on Location/Timezone)
  displayService.showOLEDStatus({"\n", "\n", "Syncing Time...", "\n", "\Timezone:", userConfig.timezone}, true);
  timeService.syncNTP(userConfig.timezone);

  // 3. Fetch Weather (Depends on Lat/Lon)
  if (userConfig.show_weather) {  
    displayService.showOLEDStatus({"\n", "\n", "Updating Weather...", "\n", "Location:", userConfig.city}, true);
    String updateTime = timeService.getCurrentTime(userConfig.time_format);
    weatherService.fetchWeather(userConfig, weatherData, updateTime);
  }

  // 4. Fetch Air Quality (Depends on Lat/Lon)
  if (userConfig.show_aqi) {  
    displayService.showOLEDStatus({"\n", "\n", "Updating AQI...", "\n", "Location:", userConfig.city}, true);
    airQualityService.fetchAirQuality(userConfig, airQualityData);
  }

  // 5. Fetch Crypto (Independent)
  if (userConfig.show_crypto) { 
    displayService.showOLEDStatus({"\n", "\n", "Updating Crypto...", "\n", "Ticker ID", String(userConfig.crypto_id)}, true);
    cryptoService.fetchPrice(userConfig.crypto_id, cryptoData);
  }

  // 6. Save Everything
  configManager.saveConfig(userConfig);

  // 7. Find the first enabled screen to show immediately
  currentScreen = SCREEN_TIME;
  for (int i = 0; i < NUM_SCREENS; i++) {
      if (displayService.isScreenEnabled(userConfig, i)) {
          currentScreen = i;
          break;
      }
  }

  lastScreenSwitch = millis();
}

// Global function wrapper for the class method
void updateAllDataCallback() {
    updateAllData();
}

void setup() {
  Serial.begin(115200);
  delay(100);
  // configManager.clearAllPreferences();

  // 1. Initialize Display and show startup message
  displayService.begin();
  delay(3000);

  // 2. Load Configuration
  displayService.showOLEDStatus({"Starting...", "Loading Config..."}, true);
  configManager.loadConfig(userConfig); 

  // 3. Connect WiFi
  // Set AP Callback to show setup instructions on OLED
  WiFiManager wm;
  // wm.resetSettings(); 
  wm.setAPCallback([](WiFiManager* m) {
    displayService.showOLEDStatus({"\n", "WiFi not connected", "\n", "Connect to WiFi:", AP_SSID, "\n", "Password:", AP_PASS}, true);
  });
  displayService.showOLEDStatus({"\n", "\n", "Connecting...", "\n", "\n", "Searching WiFi..."}, true);

  if (wm.autoConnect(AP_SSID, AP_PASS)) {
    Serial.println("WiFi Connected!"); 
    Serial.print("IP Address: "); 
    Serial.println(WiFi.localIP()); 
    displayService.showOLEDStatus({"\n", "Connected to WiFi!", "\n", "Web Panel:", WiFi.localIP().toString(), "\n", "\n", "Loading..."}, true);
    delay(3000); 

    // 4. Initial Data Fetch
    updateAllData(); 
  } else {
    Serial.println("Failed to connect and timed out. Staying in AP Mode.");
    displayService.showOLEDStatus({"\n", "Connect Failed!", "\n", "Use Web Panel to set WiFi."}, true);
  }

  // 5. Initialize Web Server
  webServerService.setSharedData(&userConfig, &weatherData, &pcStats, &cryptoData, &airQualityData);
  webServerService.begin();
}

void loop() {
  webServerService.handleClient();

  if (userConfig.show_pc) {
    pcMonitorService.handleSerial(pcStats);
  }
  
  // 1. Scheduled Data Refresh
  static unsigned long lastDataUpdate = 0;
  if (millis() - lastDataUpdate > userConfig.refresh_interval_min * 60 * 1000) {
    if (userConfig.show_weather) {
      weatherService.fetchWeather(userConfig, weatherData, timeService.getCurrentTime(userConfig.time_format));
    }

    if (userConfig.show_aqi) {
      airQualityService.fetchAirQuality(userConfig, airQualityData);
    }

    if (userConfig.show_crypto) {
      cryptoService.fetchPrice(userConfig.crypto_id, cryptoData);
    }

    lastDataUpdate = millis();
  }

  // 2. Universal Screen Switching Logic
  unsigned long intervalMs = userConfig.screen_interval_sec * 1000;

  if (millis() - lastScreenSwitch >= intervalMs) {
    int startScreen = currentScreen;
    bool foundVisible = false;

    do {
        currentScreen++;
        if (currentScreen >= NUM_SCREENS) {
            currentScreen = 0;
        }
        
        if (displayService.isScreenEnabled(userConfig, currentScreen)) {
            foundVisible = true;
            break;
        }

    } while (currentScreen != startScreen);

    if (!foundVisible) {
        currentScreen = SCREEN_TIME;
    }

    lastScreenSwitch = millis();
  }

  // 3. Screen Redraw Logic (Every 1 Second)
  static unsigned long lastScreenUpdate = 0;
  if (millis() - lastScreenUpdate >= 1000) { 
    if (WiFi.status() == WL_CONNECTED) { 
      switch(currentScreen) {
        case SCREEN_TIME:
          displayService.drawTimeScreen(userConfig, timeService.getCurrentTimeShort(userConfig.time_format), timeService.getFullDate());
          break;

        case SCREEN_WEATHER:
          displayService.drawWeatherScreen(userConfig, weatherData, timeService.getCurrentTimeShort(userConfig.time_format));
          break;

        case SCREEN_AIR_QUALITY:
          displayService.drawAQIScreen(userConfig, airQualityData, timeService.getCurrentTimeShort(userConfig.time_format));
          break;
          
        case SCREEN_PC_MONITOR:
          displayService.drawPcScreen(pcStats);
          break;

        case SCREEN_CRYPTO:
          displayService.drawCryptoScreen(cryptoData);
          break;
      }
    }
    lastScreenUpdate = millis();
  }
}