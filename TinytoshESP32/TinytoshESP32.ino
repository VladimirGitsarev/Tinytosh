#include <WiFi.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <OneButton.h>
#include "structs.h"
#include "images.h"
#include "ConfigManager.h"
#include "TimeService.h"
#include "CalendarService.h"
#include "WeatherService.h"
#include "AirQualityService.h"
#include "DisplayService.h"
#include "WebServerService.h"
#include "CryptoService.h"
#include "CurrencyService.h"
#include "StockService.h"
#include "PcMonitorService.h"
#include "BambuService.h"

// Global Constants
const char* AP_SSID = "Tinytosh";
const char* AP_PASS = "Tinytosh";
const char* PREF_NAMESPACE = "tinytosh_config";

// Timing & Night Mode Constants
const unsigned long NORMAL_REFRESH_MS = 1000;
const unsigned long NIGHT_DIM_REFRESH_MS = 10000;
const unsigned long NIGHT_DISPLAY_OFF_REFRESH_MS = 60000;
const unsigned long NIGHT_WAKE_DURATION_MS = 30000;
const int NIGHT_DATA_INTERVAL_MULTIPLIER = 10;
const int CONTRAST_DIM = 1;
const int CONTRAST_MAX = 255;

// TTP223 Button Settings
const int DEFAULT_TOUCH_PIN = 10;
const int MIN_TOUCH_PIN = 0;
const int MAX_TOUCH_PIN = 21;

// Global Data Structure
AppState appState;

// Forward declaration of callback for WebServerService
void updateAllDataCallback();

// Service Instances
ConfigManager configManager(PREF_NAMESPACE);
TimeService timeService;
CalendarService calendarService;
WeatherService weatherService;
AirQualityService airQualityService;
DisplayService displayService(128, 64, -1);
WebServerService webServerService(80, updateAllDataCallback);
CryptoService cryptoService;
CurrencyService currencyService;
StockService stockService;
PcMonitorService pcMonitorService;
BambuService bambuService;

unsigned long lastScreenSwitch = 0;
int currentScreen = 0;
bool nightModeLatched = false;

OneButton button;
unsigned long lastInteractionTime = 0; 
unsigned long lastScreenUpdate = 0;

// Helper Functions

void drawCurrentScreen() {
  displayService.drawScreen(currentScreen, appState, timeService);
}

void switchToNextScreen() {
  int startScreen = currentScreen;
  int nextScreenCandidate = currentScreen;
  bool foundVisible = false;

  // 1. Find the current screen's position index in the order array
  int currentIndex = 0;
  for (int i = 0; i < NUM_SCREENS; i++) {
    if (appState.config.screen_order[i] == currentScreen) {
      currentIndex = i;
      break;
    }
  }

  // 2. Loop forward through the array to find the next enabled screen
  int checkIndex = currentIndex;
  do {
    checkIndex++;
    if (checkIndex >= NUM_SCREENS) checkIndex = 0;

    int candidateId = appState.config.screen_order[checkIndex];
    
    if (displayService.isScreenEnabled(appState, candidateId)) {
      nextScreenCandidate = candidateId;
      foundVisible = true;
      break;
    }
  } while (checkIndex != currentIndex);

  if (!foundVisible || currentScreen == nextScreenCandidate) return;

  // 3. Animate and switch using the newly discovered screen ID
  displayService.animateTransition(currentScreen, nextScreenCandidate, appState, timeService);
  currentScreen = nextScreenCandidate;
}

int getFirstEnabledScreen() {
  for (int i = 0; i < NUM_SCREENS; i++) {
    int screenId = appState.config.screen_order[i];
    if (displayService.isScreenEnabled(appState, screenId)) {
      return screenId;
    }
  }
  return appState.config.screen_order[0];
}

bool isTimeInWindow(int currentMins, String startStr, String endStr) {
  int startH = startStr.substring(0, 2).toInt();
  int startM = startStr.substring(3, 5).toInt();
  int startMins = startH * 60 + startM;

  int endH = endStr.substring(0, 2).toInt();
  int endM = endStr.substring(3, 5).toInt();
  int endMins = endH * 60 + endM;

  if (startMins < endMins) return (currentMins >= startMins && currentMins < endMins);
  else return (currentMins >= startMins || currentMins < endMins);
}

int getActiveNightAction() {
  if (!appState.config.night_mode) return -1;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return -1;
  int currentMins = timeinfo.tm_hour * 60 + timeinfo.tm_min;

  if (appState.config.night_action == 3) {
    if (isTimeInWindow(currentMins, appState.config.night_start, appState.config.night_end)) return 2; // In the OFF window
    if (isTimeInWindow(currentMins, appState.config.night_dim_start, appState.config.night_end)) return 1; // In the DIM window
    return -1;
  }

  if (isTimeInWindow(currentMins, appState.config.night_start, appState.config.night_end)) {
    return appState.config.night_action;
  }
  
  return -1;
}

// Core Application Logic

void handleSingleClick() {
  int activeAction = getActiveNightAction();
  bool wasScreenOff = (nightModeLatched && activeAction == 2 && (millis() - lastInteractionTime >= NIGHT_WAKE_DURATION_MS));

  if (wasScreenOff) {
    Serial.println("🌙 Night Mode: Waking display temporarily on Primary Screen.");
    currentScreen = getFirstEnabledScreen();
    
    displayService.display.ssd1306_command(SSD1306_SETCONTRAST);
    displayService.display.ssd1306_command(CONTRAST_DIM);
  } else {
    Serial.println("👆 Button Pressed: Switching Screen");
    if (nightModeLatched) {
      displayService.display.ssd1306_command(SSD1306_SETCONTRAST);
      displayService.display.ssd1306_command((activeAction == 0) ? CONTRAST_MAX : CONTRAST_DIM);
    }
    switchToNextScreen();
  }
  
  lastScreenSwitch = millis();
  lastInteractionTime = millis();
  lastScreenUpdate = 0;
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
  
  lastScreenUpdate = 0; 
  lastScreenSwitch = millis(); 
  lastInteractionTime = millis();
}

int sanitizeTouchPin(int pin) {
  if (pin < MIN_TOUCH_PIN || pin > MAX_TOUCH_PIN) {
    return DEFAULT_TOUCH_PIN;
  }
  return pin;
}

void configureTouchButton() {
  appState.config.touch_pin = sanitizeTouchPin(appState.config.touch_pin);
  button.setup(appState.config.touch_pin, INPUT, false);
  button.attachClick(handleSingleClick);
  button.attachLongPressStart(handleLongPress);
  button.setDebounceMs(50);
  button.setClickMs(100);
  button.setPressMs(750);
  button.reset();
  Serial.printf("TTP223 touch sensor configured on GPIO %d\n", appState.config.touch_pin);
}

void updateAllData() {
  nightModeLatched = false;

  // 1. Location Detection
  if (appState.config.auto_detect) {
    displayService.showOLEDStatus({"\n", "\n", "Detecting Location...", "\n", "Please wait..."}, true);
    if (timeService.fetchLocationData(appState.config)) {
      Serial.println("Location updated via IP");
    }
  }

  // 2. Sync Time (Depends on Location/Timezone)
  displayService.showOLEDStatus({"\n", "\n", "Syncing Time...", "\n", "Timezone:", appState.config.timezone}, true);
  timeService.syncNTP(appState.config.timezone);

  // 3. Fetch Calendar (Holidays - Depends on Country (Country Code))
  if (appState.config.show_calendar && appState.config.calendar_show_holidays && !appState.calendar.updated) {  
    displayService.showOLEDStatus({"\n", "\n", "Updating Calendar...", "\n", "Country:", appState.config.country}, true);
    calendarService.fetchHolidays(appState.config.country_code, appState.calendar);
  }

  // 4. Fetch Weather (Depends on Lat/Lon)
  if (appState.config.show_weather) {  
    displayService.showOLEDStatus({"\n", "\n", "Updating Weather...", "\n", "Location:", appState.config.city}, true);
    String updateTime = timeService.getCurrentTime(appState.config.time_format);
    weatherService.fetchWeather(appState.config, appState.weather, updateTime);
  }

  // 5. Fetch Air Quality (Depends on Lat/Lon)
  if (appState.config.show_aqi) {  
    displayService.showOLEDStatus({"\n", "\n", "Updating AQI...", "\n", "Location:", appState.config.city}, true);
    airQualityService.fetchAirQuality(appState.config, appState.aqi);
  }

  // 6. Fetch Stocks (Independent)
  if (appState.config.show_stock) {
    displayService.showOLEDStatus({"\n", "\n", "Updating Stocks...", "\n", "Stock:", appState.config.stock_symbol}, true);
    stockService.fetchStock(appState.config.stock_symbol, appState.stock);
  }

  // 7. Fetch Crypto (Independent)
  if (appState.config.show_crypto) { 
    displayService.showOLEDStatus({"\n", "\n", "Updating Crypto...", "\n", "Ticker ID:", String(appState.config.crypto_id)}, true);
    cryptoService.fetchPrice(appState.config.crypto_id, appState.crypto);
  }

  // 8. Fetch Currency (Independent)
  if (appState.config.show_currency) { 
    String baseUpper = String(appState.config.currency_base);
    baseUpper.toUpperCase();
    String targetUpper = String(appState.config.currency_target);
    targetUpper.toUpperCase();

    displayService.showOLEDStatus({"\n", "\n", "Updating Currency...", "\n", "Currencies:", baseUpper + " -> " + targetUpper}, true);
    currencyService.fetchRate(String(appState.config.currency_base), String(appState.config.currency_target), appState.currency);
  }

  // 9. Save Everything
  configManager.saveConfig(appState.config);

  // 10. Find the first enabled screen to show immediately
  currentScreen = getFirstEnabledScreen();
  lastScreenSwitch = millis();
}

// Global function wrapper for the class method
void updateAllDataCallback() {
  configureTouchButton();
  updateAllData();
}

void setup() {
  Serial.setRxBufferSize(1024);
  Serial.begin(115200);
  delay(100);
  // configManager.clearAllPreferences();

  // 1. Initialize Display and show startup message
  displayService.begin();
  delay(3000);

  // 2. Load Configuration
  displayService.showOLEDStatus({"\n", "\n", "Starting...", "\n", "\n", "Loading Config..."}, true);
  configManager.loadConfig(appState.config); 
  configureTouchButton();
  bambuService.begin(&appState.config, &appState.bambu);

  // 3. Connect WiFi and set device info
  WiFiManager wm;
  wm.setConnectTimeout(15);
  wm.setConnectRetries(3);
  // wm.resetSettings();
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

    // 4. Initial Data Fetch
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
  button.tick();

  if (pcMonitorService.handleSerial(appState)) {
    Serial.println("Config updated via USB! Saving and applying...");
    configureTouchButton();
    configManager.saveConfig(appState.config);
    updateAllData();
  }

  // 1. Night Latch Logic
  int activeAction = getActiveNightAction();
  bool nightScheduleActive = (activeAction != -1);

  if (!nightScheduleActive) {
    if (nightModeLatched) {
      Serial.println("☀️ Morning reached: Exiting Night Mode and resuming normal operation.");
      nightModeLatched = false;
      
      // If we are exiting mode 2 or 3 (display was off), reset to the main screen
      if (appState.config.night_action >= 2) {
        currentScreen = getFirstEnabledScreen();
      }
      
      lastScreenUpdate = 0;        
      lastScreenSwitch = millis();
    }
  } else if (!nightModeLatched) {
    if (currentScreen == getFirstEnabledScreen()) {
      Serial.println("🌙 Night Mode Latched: Reached the primary screen. Applying night settings.");
      nightModeLatched = true;
      lastScreenUpdate = 0; 
      lastInteractionTime = millis() - NIGHT_WAKE_DURATION_MS;
    }
  }

  // 2. Scheduled Data Refresh
  static unsigned long lastDataUpdate = 0;
  unsigned long dataInterval = appState.config.refresh_interval_min * 60 * 1000;
  if (nightModeLatched) {
    dataInterval *= NIGHT_DATA_INTERVAL_MULTIPLIER;
  }

  if (millis() - lastDataUpdate > dataInterval) {
    if (appState.config.show_weather) weatherService.fetchWeather(appState.config, appState.weather, timeService.getCurrentTime(appState.config.time_format));
    if (appState.config.show_aqi) airQualityService.fetchAirQuality(appState.config, appState.aqi);
    if (appState.config.show_crypto) cryptoService.fetchPrice(appState.config.crypto_id, appState.crypto);
    if (appState.config.show_currency) currencyService.fetchRate(appState.config.currency_base, appState.config.currency_target, appState.currency);
    if (appState.config.show_stock) stockService.fetchStock(appState.config.stock_symbol, appState.stock);
    if (appState.config.show_calendar && appState.config.calendar_show_holidays && !appState.calendar.updated) calendarService.fetchHolidays(appState.config.country_code, appState.calendar);
    
    lastDataUpdate = millis();
  }

  // 3. Auto Screen Switching Logic
  if (appState.config.screen_auto_cycle && !nightModeLatched) {
    unsigned long intervalMs = appState.config.screen_interval_sec * 1000;
    if (millis() - lastScreenSwitch >= intervalMs) {
      switchToNextScreen();
      lastScreenSwitch = millis();
    }
  }

  // 4. Screen Redraw & Visual Action Logic
  static bool screenClearedForNight = false;
  bool isScreenOffAction = (nightModeLatched && activeAction == 2);
  bool isTemporarilyAwake = isScreenOffAction && (millis() - lastInteractionTime < NIGHT_WAKE_DURATION_MS);
  bool shouldDrawScreen = !isScreenOffAction || isTemporarilyAwake;

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
    
    unsigned long refreshInterval = NORMAL_REFRESH_MS;
    if (nightModeLatched) {
      refreshInterval = (activeAction == 2) ? NIGHT_DISPLAY_OFF_REFRESH_MS : NIGHT_DIM_REFRESH_MS;
    }

    if (millis() - lastScreenUpdate >= refreshInterval || lastScreenUpdate == 0) { 
        
      if (nightModeLatched) {
        if (activeAction == 1 || isTemporarilyAwake) {
          displayService.display.ssd1306_command(SSD1306_SETCONTRAST);
          displayService.display.ssd1306_command(CONTRAST_DIM); 
        } else if (activeAction == 0) {
          displayService.display.ssd1306_command(SSD1306_SETCONTRAST);
          displayService.display.ssd1306_command(CONTRAST_MAX);
        }
      } else {
        displayService.display.ssd1306_command(SSD1306_SETCONTRAST);
        displayService.display.ssd1306_command(CONTRAST_MAX);
      }

      drawCurrentScreen();
      displayService.display.display();
      lastScreenUpdate = millis();
    }
  }
}
