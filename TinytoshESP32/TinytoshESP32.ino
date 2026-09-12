#include <WiFi.h>
#include <WiFiManager.h>

#include "AirQualityService.h"
#include "BambuService.h"
#include "CalendarService.h"
#include "ConfigManager.h"
#include "CryptoService.h"
#include "CurrencyService.h"
#include "DataSyncService.h"
#include "DaylightService.h"
#include "DisplayService.h"
#include "HardwareService.h"
#include "images.h"
#include "MoonService.h"
#include "NightModeService.h"
#include "PcMonitorService.h"
#include "PopulationService.h"
#include "StockService.h"
#include "structs.h"
#include "TimeService.h"
#include "WeatherService.h"
#include "WebServerService.h"

// Global Constants
const char* AP_SSID = "Tinytosh";
const char* AP_PASS = "Tinytosh";
const char* PREF_NAMESPACE = "tinytosh_config";

// Global Data Structure
AppState appState;

// Forward declarations of callbacks
void updateAllDataCallback();
void handleSingleClick();
void handleDoubleClick();
void handleLongPress();

// Service Instances
ConfigManager configManager(PREF_NAMESPACE);
DisplayService displayService(128, 64, -1);
WebServerService webServerService(80, updateAllDataCallback);
HardwareService hardwareService(handleSingleClick, handleDoubleClick, handleLongPress);
TimeService timeService;
CalendarService calendarService;
WeatherService weatherService;
AirQualityService airQualityService;
DaylightService daylightService;
MoonService moonService;
PopulationService populationService;
CryptoService cryptoService;
CurrencyService currencyService;
StockService stockService;
PcMonitorService pcMonitorService;
BambuService bambuService;
DataSyncService dataSyncService;
NightModeService nightModeService;

unsigned long lastScreenSwitch = 0;

// Core Application Logic

void handleScreenNavigation(bool goToPrevious) {
  int activeAction = TimeService::getActiveNightAction(appState.config);
  bool wasScreenOff = nightModeService.wasScreenOff(activeAction);

  if (wasScreenOff) {
    Serial.println("🌙 Night Mode: Waking display temporarily on Primary Screen.");
    displayService.jumpToFirstEnabledScreen(appState);
    displayService.setContrast(true);
  } else {
    if (nightModeService.isLatched()) {
      displayService.setContrast(activeAction != 0);
    }
    if (goToPrevious) {
      Serial.println("👆👆 Double Click: Switching to Previous Screen");
      displayService.switchToPreviousScreen(appState);
    } else {
      Serial.println("👆 Single Click: Switching to Next Screen");
      displayService.switchToNextScreen(appState);
    }
  }

  lastScreenSwitch = millis();
  nightModeService.recordInteraction();
}

void handleSingleClick() {
  handleScreenNavigation(false);
}

void handleDoubleClick() {
  handleScreenNavigation(true);
}

void handleLongPress() {
  appState.config.screen_auto_cycle = !appState.config.screen_auto_cycle;

  if (appState.config.screen_auto_cycle) {
    Serial.println("🔄 Auto Cycle: ENABLED");
    displayService.drawInfoScreen(icon_unlock, "Auto Cycle On");
    displayService.display.display();
  } else {
    Serial.println("🔒 Auto Cycle: DISABLED (Screen Locked)");
    displayService.drawInfoScreen(icon_lock, "Auto Cycle Off");
    displayService.display.display();
  }
  
  configManager.saveConfig(appState.config);

  delay(1000);

  lastScreenSwitch = millis();
  nightModeService.recordInteraction();
}

// Full Data Sync
void updateAllData() {
  nightModeService.reset();
  dataSyncService.runFullSync(appState);
  displayService.jumpToFirstEnabledScreen(appState);
  lastScreenSwitch = millis();
}

// Global function wrapper for the class method
void updateAllDataCallback() {
  updateAllData();
}

void setup() {
  Serial.setRxBufferSize(1024);
  Serial.begin(115200);
  delay(100);

  configManager.loadConfig(appState.config);
  hardwareService.begin(appState.config);

  displayService.begin(appState.config.sda_pin, appState.config.scl_pin);
  delay(3000);

  displayService.showOLEDStatus({"\n", "\n", "Starting...", "\n", "\n", "Config Loaded!"}, true);
  bambuService.begin(&appState.config, &appState.bambu);

  WiFiManager wm;
  wm.setConnectTimeout(15);
  wm.setConnectRetries(3);

  wm.setAPCallback([](WiFiManager* m) {
    displayService.showOLEDStatus({"\n", "WiFi not connected", "\n", "Connect to WiFi:", AP_SSID, "\n", "Password:", AP_PASS}, true);
  });

  displayService.showOLEDStatus({"\n", "\n", "Connecting...", "\n", "\n", "Searching WiFi..."}, true);

  if (wm.autoConnect(AP_SSID, AP_PASS)) {
    String ipAddress = WiFi.localIP().toString();
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    String uniqueName = "tinytosh-" + mac.substring(8);
    uniqueName.toLowerCase();

    Serial.println("WiFi Connected!"); 
    Serial.print("IP Address: "); 
    Serial.println(ipAddress);

    appState.config.device_id = uniqueName;
    appState.config.ip_address = ipAddress;
    displayService.showOLEDStatus({
        "Connected to WiFi!", 
        "", 
        "IP: " + ipAddress, 
        "Name: " + uniqueName, 
        "", 
        "Loading..."
    }, true);

    delay(3000); 

    // 4. Initial Data Fetch (Synchronous)
    updateAllData();

  } else {
    Serial.println("Failed to connect and timed out. Staying in AP Mode.");
    displayService.showOLEDStatus({"\n", "Connect Failed!", "\n", "Use Web Panel to set WiFi."}, true);
  }

  // 5. Initialize Web Server
  webServerService.setAppState(&appState);
  webServerService.begin();
}

void loop() {
  webServerService.handleClient();
  bambuService.loop();
  hardwareService.tick();

  if (pcMonitorService.handleSerial(appState)) {
    Serial.println("Config updated via USB! Saving and applying...");
    configManager.saveConfig(appState.config);
    updateAllData();
  }

  // 1. Night Latch Logic
  int activeAction = TimeService::getActiveNightAction(appState.config);
  bool justExitedNightMode = nightModeService.update(activeAction, displayService.isOnFirstEnabledScreen(appState));

  if (justExitedNightMode) {
    // If we are exiting mode 2 or 3 (display was off), reset to the main screen
    if (appState.config.night_action >= 2) {
      displayService.jumpToFirstEnabledScreen(appState);
    }
    lastScreenSwitch = millis();
  }

  // 2. Scheduled Data Refresh (Non-Blocking via FreeRTOS Task)
  dataSyncService.maybeStartBackgroundSync(appState, nightModeService.isLatched());

  // 3. Auto Screen Switching Logic
  if (appState.config.screen_auto_cycle && !nightModeService.isLatched()) {
    unsigned long intervalMs = appState.config.screen_interval_sec * 1000;

    if (millis() - lastScreenSwitch >= intervalMs) {
      displayService.switchToNextScreen(appState);
      lastScreenSwitch = millis();
    }
  }

  // 4. Screen Redraw & Visual Action Logic
  static bool screenClearedForNight = false;

  bool isTemporarilyAwake = nightModeService.isTemporarilyAwake(activeAction);
  bool shouldDrawScreen = !nightModeService.isScreenOffAction(activeAction) || isTemporarilyAwake;

  if (!shouldDrawScreen) {
    if (!screenClearedForNight) {
      displayService.display.clearDisplay();
      displayService.display.display();
      screenClearedForNight = true;
      Serial.println("💤 Night Mode: Display turned OFF to save power. Waiting for interaction or morning.");
    }
  } else {
    if (screenClearedForNight) {
      screenClearedForNight = false;
      Serial.println("💡 Night Mode: Display turned back ON.");
    }

    unsigned long refreshInterval = nightModeService.getRefreshIntervalMs(activeAction);

    if (nightModeService.isRedrawDue(refreshInterval)) {
      if (nightModeService.isLatched()) {
        if (activeAction == 1 || isTemporarilyAwake) {
          displayService.setContrast(true);
        } else if (activeAction == 0) {
          displayService.setContrast(false);
        }
      } else {
        displayService.setContrast(false);
      }

      displayService.drawCurrentScreen(appState);
      displayService.display.display();
      nightModeService.markRedrawn();
    }
  }
}