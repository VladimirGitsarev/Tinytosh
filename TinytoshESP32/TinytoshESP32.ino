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
#include "DaylightService.h"
#include "DisplayService.h"
#include "WebServerService.h"
#include "CryptoService.h"
#include "CurrencyService.h"
#include "StockService.h"
#include "PcMonitorService.h"
#include "BambuService.h"
#include "FocusService.h"

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

// Hardware Pins
const int DEFAULT_TOUCH_PIN = 10;
const int DEFAULT_SDA_PIN = 8;
const int DEFAULT_SCL_PIN = 9;
const int MIN_GPIO_PIN = 0;
const int MAX_GPIO_PIN = 21;

// Global Data Structure
AppState appState;

// Forward declaration of callback for WebServerService
void updateAllDataCallback();

// Service Instances
ConfigManager configManager(PREF_NAMESPACE);
DisplayService displayService(128, 64, -1);
WebServerService webServerService(80, updateAllDataCallback);
TimeService timeService;
CalendarService calendarService;
WeatherService weatherService;
AirQualityService airQualityService;
DaylightService daylightService;
CryptoService cryptoService;
CurrencyService currencyService;
StockService stockService;
PcMonitorService pcMonitorService;
BambuService bambuService;
FocusService focusService;

unsigned long lastScreenSwitch = 0;
int currentScreen = 0;
int currentSubScreen = 0;
bool nightModeLatched = false;

OneButton button;
unsigned long lastInteractionTime = 0; 
unsigned long lastScreenUpdate = 0;

// Background Task & Sync Trackers
bool isBackgroundUpdating = false;
TaskHandle_t bgUpdateTaskHandle = NULL;
bool pendingDataUpdate = false;

// Helper Functions

void drawCurrentScreen() {
  displayService.drawScreen(currentScreen, appState, currentSubScreen);
}

void switchToNextScreen() {
  if (currentScreen == SCREEN_STOCK && currentSubScreen + 1 < appState.config.stock_count) {
      currentSubScreen++;
      displayService.animateTransition(currentScreen, currentSubScreen - 1, currentScreen, currentSubScreen, appState);
      return;
  }
  if (currentScreen == SCREEN_CRYPTO && currentSubScreen + 1 < appState.config.crypto_count) {
      currentSubScreen++;
      displayService.animateTransition(currentScreen, currentSubScreen - 1, currentScreen, currentSubScreen, appState);
      return;
  }
  if (currentScreen == SCREEN_CURRENCY && currentSubScreen + 1 < appState.config.currency_count) {
      currentSubScreen++;
      displayService.animateTransition(currentScreen, currentSubScreen - 1, currentScreen, currentSubScreen, appState);
      return;
  }

  int oldScreen = currentScreen;
  int oldSubScreen = currentSubScreen;
  currentSubScreen = 0; 

  int currentIndex = 0;
  for (int i = 0; i < NUM_SCREENS; i++) {
    if (appState.config.screen_order[i] == currentScreen) {
      currentIndex = i; break;
    }
  }

  int checkIndex = currentIndex;
  int nextScreenCandidate = currentScreen;
  do {
    checkIndex++;
    if (checkIndex >= NUM_SCREENS) checkIndex = 0;
    int candidateId = appState.config.screen_order[checkIndex];
    if (displayService.isScreenEnabled(appState, candidateId)) {
      nextScreenCandidate = candidateId;
      break;
    }
  } while (checkIndex != currentIndex);

  if (oldScreen == nextScreenCandidate && oldSubScreen == 0) return;

  displayService.animateTransition(oldScreen, oldSubScreen, nextScreenCandidate, 0, appState);
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
    if (isTimeInWindow(currentMins, appState.config.night_start, appState.config.night_end)) return 2;
    if (isTimeInWindow(currentMins, appState.config.night_dim_start, appState.config.night_end)) return 1;
    return -1;
  }

  if (isTimeInWindow(currentMins, appState.config.night_start, appState.config.night_end)) {
    return appState.config.night_action;
  }
  
  return -1;
}

void validatePins(Config& config) {
  bool used_pins[MAX_GPIO_PIN + 1] = {false};

  auto claimPin = [&](int &pin, int default_pin) {
    
    // 1. Sanitize bounds
    if (pin < MIN_GPIO_PIN || pin > MAX_GPIO_PIN) {
      pin = default_pin;
    }

    // 2. Check for collision
    if (used_pins[pin]) {
      pin = default_pin;
      
      // 3. Find the absolute first available free pin
      if (used_pins[pin]) {
        for (int i = MIN_GPIO_PIN; i <= MAX_GPIO_PIN; i++) {
          if (!used_pins[i]) {
            pin = i;
            break;
          }
        }
      }
    }
    
    // 4. Mark the final chosen pin as claimed
    used_pins[pin] = true;
  };

  claimPin(config.sda_pin, DEFAULT_SDA_PIN);
  claimPin(config.scl_pin, DEFAULT_SCL_PIN);
  claimPin(config.touch_pin, DEFAULT_TOUCH_PIN);
}

void configureHardware() {
  validatePins(appState.config);
  
  button.setup(appState.config.touch_pin, INPUT, false);
  button.attachClick(handleSingleClick);
  button.attachDoubleClick(handleFocusDoubleClick);
  button.attachMultiClick(handleFocusMultiClick);
  button.attachLongPressStart(handleLongPress);
  button.setDebounceTicks(50); 
  button.setClickTicks(300); // Enough time for double/triple presses to register
  button.setPressTicks(750);
  button.reset();
  
  Serial.printf("Hardware Configured: SDA=%d, SCL=%d, TOUCH=%d\n", appState.config.sda_pin, appState.config.scl_pin, appState.config.touch_pin);
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

void handleFocusDoubleClick() {
  if (!focusService.onDoublePress(currentScreen, appState.config, appState.focus)) return;
  configManager.saveConfig(appState.config);

  lastScreenSwitch = millis();
  lastInteractionTime = millis();
  lastScreenUpdate = 0;
}

void handleFocusMultiClick() {
  if (button.getNumberClicks() < 3) return;
  if (!focusService.onTriplePress(currentScreen, appState.focus)) return;

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

  struct tm timeinfo;
  getLocalTime(&timeinfo);
  int current_year = timeinfo.tm_year + 1900;
  int current_yday = timeinfo.tm_yday;

  // 3. Fetch Calendar (Holidays - Depends on Country (Country Code))
  if (appState.config.show_calendar && appState.config.calendar_show_holidays && appState.calendar.last_fetch_year != current_year) {  
    displayService.showOLEDStatus({"\n", "\n", "Updating Calendar...", "\n", "Country:", appState.config.country}, true);
    calendarService.fetchHolidays(appState.config.country_code, appState.calendar);
  }

  // 4. Fetch Weather (Depends on Lat/Lon)
  if (appState.config.show_weather) {  
    displayService.showOLEDStatus({"\n", "\n", "Updating Weather...", "\n", "Location:", appState.config.city}, true);
    String updateTime = TimeService::getCurrentTime(appState.config.time_format);
    weatherService.fetchWeather(appState.config, appState.weather, updateTime);
  }

  // 5. Fetch Air Quality (Depends on Lat/Lon)
  if (appState.config.show_aqi) {  
    displayService.showOLEDStatus({"\n", "\n", "Updating AQI...", "\n", "Location:", appState.config.city}, true);
    airQualityService.fetchAirQuality(appState.config, appState.aqi);
  }

  // 6. Fetch Daylight (Depends on Lat/Lon)
  if (appState.config.show_daylight && appState.daylight.last_fetch_yday != current_yday) {  
    displayService.showOLEDStatus({"\n", "\n", "Updating Daylight...", "\n", "Location:", appState.config.city}, true);
    daylightService.fetchDaylight(appState.config, appState.daylight);
  }

  // 7. Fetch Stocks (Independent)
  if (appState.config.show_stock) {
    for (int i = 0; i < appState.config.stock_count; i++) {
      displayService.showOLEDStatus({"\n", "\n", "Updating Stocks...", "\n", "Stock:", appState.config.stock_symbols[i]}, true);
      stockService.fetchStock(appState.config.stock_symbols[i], appState.stocks[i]);
    }
  }

  // 8. Fetch Crypto (Independent)
  if (appState.config.show_crypto) { 
    for (int i = 0; i < appState.config.crypto_count; i++) {
      displayService.showOLEDStatus({"\n", "\n", "Updating Crypto...", "\n", "Ticker ID:", String(appState.config.crypto_ids[i])}, true);
      cryptoService.fetchPrice(appState.config.crypto_ids[i], appState.cryptos[i]);
    }
  }

  // 9. Fetch Currency (Independent)
  if (appState.config.show_currency) { 
    for (int i = 0; i < appState.config.currency_count; i++) {
      String baseUpper = String(appState.config.currency_bases[i]);
      baseUpper.toUpperCase();
      String targetUpper = String(appState.config.currency_targets[i]);
      targetUpper.toUpperCase();

      displayService.showOLEDStatus({"\n", "\n", "Updating Currency...", "\n", "Currencies:", baseUpper + " -> " + targetUpper}, true);
      currencyService.fetchRate(appState.config.currency_bases[i], appState.config.currency_targets[i], appState.currencies[i]);
    }
  }

  displayService.showOLEDStatus({"\n", "\n", "Data Updated", "\n", "\n", "Tinytosh is Ready", "\n", "\n", "Welcome!"}, true);

  // 10. Save Everything
  configManager.saveConfig(appState.config);

  // 11. Find the first enabled screen to show immediately
  currentScreen = getFirstEnabledScreen();
  lastScreenSwitch = millis();
}

// Global function wrapper for the class method
void updateAllDataCallback() {
  updateAllData();
}

// Background Task for Scheduled Updates
void backgroundDataFetchTask(void* parameter) {
  Serial.println("Background Task: Starting API updates...");

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Background Task: WiFi offline. Aborting updates.");
    isBackgroundUpdating = false;
    bgUpdateTaskHandle = NULL;
    vTaskDelete(NULL);
    return;
  }
  
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  int current_year = timeinfo.tm_year + 1900;
  int current_yday = timeinfo.tm_yday;

  if (appState.config.show_weather) weatherService.fetchWeather(appState.config, appState.weather, TimeService::getCurrentTime(appState.config.time_format));
  if (appState.config.show_aqi) airQualityService.fetchAirQuality(appState.config, appState.aqi);
  if (appState.config.show_daylight && appState.daylight.last_fetch_yday != current_yday) daylightService.fetchDaylight(appState.config, appState.daylight);
  if (appState.config.show_crypto) {
    for (int i = 0; i < appState.config.crypto_count; i++) cryptoService.fetchPrice(appState.config.crypto_ids[i], appState.cryptos[i]);
  }
  if (appState.config.show_currency) {
    for (int i = 0; i < appState.config.currency_count; i++) currencyService.fetchRate(appState.config.currency_bases[i], appState.config.currency_targets[i], appState.currencies[i]);
  }
  if (appState.config.show_stock) {
    for (int i = 0; i < appState.config.stock_count; i++) stockService.fetchStock(appState.config.stock_symbols[i], appState.stocks[i]);
  }
  if (appState.config.show_calendar && appState.config.calendar_show_holidays && appState.calendar.last_fetch_year != current_year) calendarService.fetchHolidays(appState.config.country_code, appState.calendar);
  
  Serial.println("Background Task: Updates complete.");
  isBackgroundUpdating = false;
  bgUpdateTaskHandle = NULL;
  vTaskDelete(NULL);
}

void setup() {
  Serial.setRxBufferSize(1024);
  Serial.begin(115200);
  delay(100);

  configManager.loadConfig(appState.config); 
  configureHardware();

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
  button.tick();

  if (pcMonitorService.handleSerial(appState)) {
    Serial.println("Config updated via USB! Saving and applying...");
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

  // 1b. Focus Mode: detect a Running session's countdown reaching zero.
  // tick() resets focus back to Idle on completion, which also re-opens screen switching.
  if (focusService.tick(appState.focus, appState.config)) {
    Serial.println("✅ Focus Session Complete!");
    displayService.playFocusCompleteAnimation(appState.config.focus_flash);
    lastScreenUpdate = 0;
    lastScreenSwitch = millis();
  }

  // 2. Scheduled Data Refresh (Non-Blocking via FreeRTOS Task)
  static unsigned long lastDataUpdate = 0;
  unsigned long dataInterval = appState.config.refresh_interval_min * 60 * 1000;
  
  if (nightModeLatched) {
    dataInterval *= NIGHT_DATA_INTERVAL_MULTIPLIER;
  }

  if (millis() - lastDataUpdate > dataInterval) {
    if (!isBackgroundUpdating) {
        isBackgroundUpdating = true;
        Serial.println("Spawning background data update task...");
        xTaskCreate(
            backgroundDataFetchTask, 
            "BgUpdateTask", 
            8192,
            NULL, 
            1,
            &bgUpdateTaskHandle
        );
    }
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