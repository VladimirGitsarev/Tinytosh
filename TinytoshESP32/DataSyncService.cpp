#include "DataSyncService.h"

#include <Arduino.h>
#include <time.h>
#include <WiFi.h>

#include "AirQualityService.h"
#include "CalendarService.h"
#include "ConfigManager.h"
#include "CryptoService.h"
#include "CurrencyService.h"
#include "DaylightService.h"
#include "DisplayService.h"
#include "MoonService.h"
#include "PopulationService.h"
#include "StockService.h"
#include "TimeService.h"
#include "WeatherService.h"


extern ConfigManager configManager;
extern DisplayService displayService;
extern TimeService timeService;
extern CalendarService calendarService;
extern WeatherService weatherService;
extern AirQualityService airQualityService;
extern DaylightService daylightService;
extern MoonService moonService;
extern PopulationService populationService;
extern StockService stockService;
extern CryptoService cryptoService;
extern CurrencyService currencyService;

unsigned long DataSyncService::getGlobalIntervalMs(const Config& config, bool nightModeLatched) const {
    unsigned long multiplier = nightModeLatched ? NIGHT_INTERVAL_MULTIPLIER : 1;
    return config.refresh_interval_min * 60000UL * multiplier;
}

bool DataSyncService::isFetchDue(unsigned long lastFetch, int customMin, const Config& config, bool nightModeLatched) const {
    unsigned long multiplier = nightModeLatched ? NIGHT_INTERVAL_MULTIPLIER : 1;
    unsigned long intervalMs = (customMin > 0) ? ((unsigned long)customMin * 60000UL * multiplier) : getGlobalIntervalMs(config, nightModeLatched);
    return (millis() - lastFetch) > intervalMs;
}

bool DataSyncService::isGlobalSyncDue(const Config& config, bool nightModeLatched) const {
    return (millis() - trackers.lastDataUpdate) > getGlobalIntervalMs(config, nightModeLatched);
}

void DataSyncService::markGlobalSynced() {
    trackers.lastDataUpdate = millis();
}

bool DataSyncService::isDue(ScreenType screen, const Config& config, bool nightModeLatched) const {
    switch (screen) {
        case SCREEN_WEATHER:
            return config.show_weather && isFetchDue(trackers.lastWeatherFetch, config.custom_weather_int_min, config, nightModeLatched);
        case SCREEN_AIR_QUALITY:
            return config.show_aqi && isFetchDue(trackers.lastAqiFetch, config.custom_aqi_int_min, config, nightModeLatched);
        case SCREEN_STOCK:
            return config.show_stock && isFetchDue(trackers.lastStockFetch, config.custom_stock_int_min, config, nightModeLatched);
        case SCREEN_CRYPTO:
            return config.show_crypto && isFetchDue(trackers.lastCryptoFetch, config.custom_crypto_int_min, config, nightModeLatched);
        case SCREEN_CURRENCY:
            return config.show_currency && isFetchDue(trackers.lastCurrencyFetch, config.custom_currency_int_min, config, nightModeLatched);
        default:
            return false;
    }
}

void DataSyncService::markFetched(ScreenType screen) {
    unsigned long now = millis();
    switch (screen) {
        case SCREEN_WEATHER: trackers.lastWeatherFetch = now; break;
        case SCREEN_AIR_QUALITY: trackers.lastAqiFetch = now; break;
        case SCREEN_STOCK: trackers.lastStockFetch = now; break;
        case SCREEN_CRYPTO: trackers.lastCryptoFetch = now; break;
        case SCREEN_CURRENCY: trackers.lastCurrencyFetch = now; break;
        default: break;
    }
}

void DataSyncService::runFullSync(AppState& state) {
    Config& config = state.config;

    // 1. Location Detection
    if (config.auto_detect) {
        displayService.showOLEDStatus({"\n", "\n", "Detecting Location...", "\n", "Please wait..."}, true);
        if (timeService.fetchLocationData(config)) {
            Serial.println("Location updated via IP");
        }
    }

    // 2. Sync Time (Depends on Location/Timezone)
    displayService.showOLEDStatus({"\n", "\n", "Syncing Time...", "\n", "Timezone:", config.timezone}, true);
    timeService.syncNTP(config.timezone);

    struct tm timeinfo;
    getLocalTime(&timeinfo);
    int current_year = timeinfo.tm_year + 1900;
    int current_yday = timeinfo.tm_yday;

    // 3. Fetch Calendar (Holidays - Depends on Country (Country Code))
    if (config.show_calendar && config.calendar_show_holidays && state.calendar.last_fetch_year != current_year) {
        displayService.showOLEDStatus({"\n", "\n", "Updating Calendar...", "\n", "Country:", config.country}, true);
        calendarService.fetchHolidays(config.country_code, state.calendar);
    }

    // 4. Fetch Weather (Depends on Lat/Lon)
    if (config.show_weather) {
        displayService.showOLEDStatus({"\n", "\n", "Updating Weather...", "\n", "Location:", config.city}, true);
        String updateTime = TimeService::getCurrentTime(config.time_format);
        weatherService.fetchWeather(config, state.weather, updateTime);
        markFetched(SCREEN_WEATHER);
    }

    // 5. Fetch Air Quality (Depends on Lat/Lon)
    if (config.show_aqi) {
        displayService.showOLEDStatus({"\n", "\n", "Updating AQI...", "\n", "Location:", config.city}, true);
        airQualityService.fetchAirQuality(config, state.aqi);
        markFetched(SCREEN_AIR_QUALITY);
    }

    // 6. Fetch Daylight (Depends on Lat/Lon)
    if (config.show_daylight && state.daylight.last_fetch_yday != current_yday) {
        displayService.showOLEDStatus({"\n", "\n", "Updating Daylight...", "\n", "Location:", config.city}, true);
        daylightService.fetchDaylight(config, state.daylight);
    }

    // 7. Fetch Moon (Depends on Lat/Lon)
    if (config.show_moon && state.moon.last_fetch_yday != current_yday) {
        displayService.showOLEDStatus({"\n", "\n", "Updating Moon...", "\n", "Location:", config.city}, true);
        moonService.fetchMoon(config, state.moon);
    }

    // 8. Fetch Population (Depends on Country Code)
    if (config.show_population && state.population.last_fetch_yday != current_yday) {
        String popLoc = (config.pop_show_world && config.pop_show_country && config.country_code != "") ? ("World + " + config.country_code) : (config.pop_show_world ? "World" : config.country_code);
        displayService.showOLEDStatus({"\n", "\n", "Updating Populace...", "\n", "Location:", popLoc}, true);
        populationService.fetchPopulation(config, state.population);
    }

    // 9. Fetch Stocks (Independent)
    if (config.show_stock) {
        for (int i = 0; i < config.stock_count; i++) {
            displayService.showOLEDStatus({"\n", "\n", "Updating Stocks...", "\n", "Stock:", config.stock_symbols[i]}, true);
            stockService.fetchStock(config.stock_symbols[i], state.stocks[i]);
        }
        markFetched(SCREEN_STOCK);
    }

    // 10. Fetch Crypto (Independent)
    if (config.show_crypto) {
        for (int i = 0; i < config.crypto_count; i++) {
            displayService.showOLEDStatus({"\n", "\n", "Updating Crypto...", "\n", "Ticker ID:", String(config.crypto_ids[i])}, true);
            cryptoService.fetchPrice(config.crypto_ids[i], state.cryptos[i]);
        }
        markFetched(SCREEN_CRYPTO);
    }

    // 11. Fetch Currency (Independent)
    if (config.show_currency) {
        for (int i = 0; i < config.currency_count; i++) {
            String baseUpper = String(config.currency_bases[i]);
            baseUpper.toUpperCase();
            String targetUpper = String(config.currency_targets[i]);
            targetUpper.toUpperCase();

            displayService.showOLEDStatus({"\n", "\n", "Updating Currency...", "\n", "Currencies:", baseUpper + " -> " + targetUpper}, true);
            currencyService.fetchRate(config.currency_bases[i], config.currency_targets[i], state.currencies[i]);
        }
        markFetched(SCREEN_CURRENCY);
    }

    displayService.showOLEDStatus({"\n", "\n", "Data Updated", "\n", "\n", "Tinytosh is Ready", "\n", "\n", "Welcome!"}, true);

    // 12. Save Everything
    configManager.saveConfig(config);

    markGlobalSynced();
}

void DataSyncService::maybeStartBackgroundSync(AppState& state, bool nightModeLatched) {
    bool dueGlobal = isGlobalSyncDue(state.config, nightModeLatched);
    bool anyScreenDue = dueGlobal
        || isDue(SCREEN_WEATHER, state.config, nightModeLatched)
        || isDue(SCREEN_AIR_QUALITY, state.config, nightModeLatched)
        || isDue(SCREEN_STOCK, state.config, nightModeLatched)
        || isDue(SCREEN_CRYPTO, state.config, nightModeLatched)
        || isDue(SCREEN_CURRENCY, state.config, nightModeLatched);

    if (!anyScreenDue) return;

    if (!isBackgroundSyncRunning) {
        isBackgroundSyncRunning = true;
        activeState = &state;
        activeNightModeLatched = nightModeLatched;

        Serial.println("Spawning background data update task...");
        xTaskCreate(
            backgroundSyncTaskTrampoline,
            "BgUpdateTask",
            8192,
            this,
            1,
            &backgroundSyncTaskHandle
        );
    }

    if (dueGlobal) markGlobalSynced();
}

void DataSyncService::backgroundSyncTaskTrampoline(void* parameter) {
    static_cast<DataSyncService*>(parameter)->runBackgroundSyncBody();
}

void DataSyncService::runBackgroundSyncBody() {
    Serial.println("Background Task: Starting API updates...");

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Background Task: WiFi offline. Aborting updates.");
        isBackgroundSyncRunning = false;
        backgroundSyncTaskHandle = NULL;
        vTaskDelete(NULL);
        return;
    }

    AppState& state = *activeState;
    Config& config = state.config;
    bool nightModeLatched = activeNightModeLatched;

    struct tm timeinfo;
    getLocalTime(&timeinfo);
    int current_year = timeinfo.tm_year + 1900;
    int current_yday = timeinfo.tm_yday;

    // Daily/yearly screens are always re-checked here; they no-op internally unless the day/year rolled over.
    if (config.show_daylight && state.daylight.last_fetch_yday != current_yday) daylightService.fetchDaylight(config, state.daylight);
    if (config.show_moon && state.moon.last_fetch_yday != current_yday) moonService.fetchMoon(config, state.moon);
    if (config.show_population && state.population.last_fetch_yday != current_yday) populationService.fetchPopulation(config, state.population);
    if (config.show_calendar && config.calendar_show_holidays && state.calendar.last_fetch_year != current_year) calendarService.fetchHolidays(config.country_code, state.calendar);

    // Custom-sync-eligible screens each check their own due status here.
    if (isDue(SCREEN_WEATHER, config, nightModeLatched)) {
        weatherService.fetchWeather(config, state.weather, TimeService::getCurrentTime(config.time_format));
        markFetched(SCREEN_WEATHER);
    }
    if (isDue(SCREEN_AIR_QUALITY, config, nightModeLatched)) {
        airQualityService.fetchAirQuality(config, state.aqi);
        markFetched(SCREEN_AIR_QUALITY);
    }
    if (isDue(SCREEN_STOCK, config, nightModeLatched)) {
        for (int i = 0; i < config.stock_count; i++) stockService.fetchStock(config.stock_symbols[i], state.stocks[i]);
        markFetched(SCREEN_STOCK);
    }
    if (isDue(SCREEN_CRYPTO, config, nightModeLatched)) {
        for (int i = 0; i < config.crypto_count; i++) cryptoService.fetchPrice(config.crypto_ids[i], state.cryptos[i]);
        markFetched(SCREEN_CRYPTO);
    }
    if (isDue(SCREEN_CURRENCY, config, nightModeLatched)) {
        for (int i = 0; i < config.currency_count; i++) currencyService.fetchRate(config.currency_bases[i], config.currency_targets[i], state.currencies[i]);
        markFetched(SCREEN_CURRENCY);
    }

    Serial.println("Background Task: Updates complete.");
    isBackgroundSyncRunning = false;
    backgroundSyncTaskHandle = NULL;
    vTaskDelete(NULL);
}
