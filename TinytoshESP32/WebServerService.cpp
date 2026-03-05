#include "WebServerService.h"
#include <ArduinoJson.h>
#include "zones.h"

String WebServerService::getCurrentTimeShort(String format) {
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char time_str[12];
    if (format == "12") {
        strftime(time_str, sizeof(time_str), "%I:%M", &timeinfo);
    } else {
        strftime(time_str, sizeof(time_str), "%H:%M", &timeinfo);
    }
    return String(time_str);
}

String WebServerService::getFullDate() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return "No Date";
    
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%A, %b %d", &timeinfo);
    return String(buffer);
}

String WebServerService::getWeatherIcon(int wmo_code) {
  if (wmo_code == 0) return "☀️"; 
  if (wmo_code == 1 || wmo_code == 2 || wmo_code == 3) return "🌤️"; 
  if (wmo_code <= 48) return "🌫️"; 
  if (wmo_code <= 55) return "🌧️"; 
  if (wmo_code <= 65) return "☔"; 
  if (wmo_code <= 75) return "❄️"; 
  if (wmo_code <= 86) return "🌨️"; 
  if (wmo_code <= 99) return "🌩️"; 
  return "❓";
}

WebServerService::WebServerService(int port, ConfigSaveCallback callback) : 
  server(port), saveCallback(callback) {}

void WebServerService::setAppState(AppState* appState) {
  state = appState;
}

void WebServerService::begin() {
  server.on("/", HTTP_GET, [this](){ this->handleRoot(); }); 
  server.on("/save", HTTP_GET, [this](){ this->handleSave(); });
  server.on("/update", HTTP_GET, [this](){ this->handleUpdate(); }); 
  server.begin();
  Serial.println("WebServerService: HTTP Server started."); 
}

void WebServerService::handleClient() {
    server.handleClient();
}

String WebServerService::generateRootPageContent() {
  String content;
  content.reserve(30000);

  Config& config = state->config;
  WeatherData& weather = state->weather;
  AirQualityData& aqi = state->aqi;
  CryptoData& crypto = state->crypto;
  CurrencyData& currency = state->currency;
  StockData& stock = state->stock;
  PcStats& pc = state->pc;
  
  bool weatherValid = !isnan(weather.temp);
  bool aqiValid = !isnan(aqi.pm25) && !isnan(aqi.pm10) && !isnan(aqi.no2);
  bool pcValid = pc.cpu_percent > 0.1; 
  bool cryptoValid = !isnan(crypto.price_usd) && crypto.price_usd > 0;
  bool currencyValid = currency.updated;
  bool stockValid = stock.updated;

  content += "<html><head><title>Tinytosh | Web Panel</title>";
  content += "<meta name='viewport' content='width=device-width, initial-scale=1'><meta charset='UTF-8'>";
  content += "<style>";
  // CSS Variables
  content += ":root { --bg: #0f172a; --card: #1e293b; --accent: #3b82f6; --text: #f1f5f9; --text-muted: #94a3b8; --border: #334155; }";

  // Global Body & Container Styles
  content += "* { box-sizing: border-box; }";
  content += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: var(--bg); color: var(--text); margin: 0; padding: 20px; line-height: 1.6; }";
  content += ".container { max-width: 800px; margin: 0 auto; }";

  // Header & Time Display
  content += ".app-header { padding-bottom: 20px; font-size: 2.5rem; color: var(--accent); font-weight: 700; text-align: center; }";
  content += "#time-display { text-align: center; color: var(--text); font-size: 4.5rem; font-weight: 800; letter-spacing: -2px; }";
  content += "#location-info { text-align: center; margin-top: 0px; color: var(--text-muted); font-weight: 400; }";

  // Panels & Tiles (Card Style)
  content += ".panel { background: var(--card); padding: 25px; border-radius: 12px; border: 1px solid var(--border); margin-bottom: 24px; }";
  content += ".header-panel { background: rgba(59, 130, 246, 0.05); border: 1px solid var(--accent);}";
  content += ".dashboard-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 15px; margin-top: 20px; }";
  content += ".dashboard-grid > div { min-width: 0; }";
  content += ".tile { background: rgba(15, 23, 42, 0.4); border: 1px dashed var(--border); padding: 20px; border-radius: 10px; text-align: center; }";
  content += ".tile-icon { font-size: 2.2rem; margin-bottom: 8px; }";
  content += ".tile-label { font-size: 0.75rem; color: var(--text-muted); text-transform: uppercase; letter-spacing: 1px; }";
  content += ".tile-value { font-size: 1.6rem; font-weight: 600; color: var(--text); }";

 // Form Elements
  content += "label { display: block; margin-top: 15px; font-weight: 600; color: var(--text); }";
  content += "input[type='text'], input[type='number'], input[type='time'], select { display: block; box-sizing: border-box !important; -webkit-box-sizing: border-box !important; width: 100% !important; min-width: 0 !important; padding: 12px; margin: 8px 0; border: 1px solid var(--border); border-radius: 6px; background-color: #0f172a; color: var(--text); font-size: 15px; }";
  content += "input[type='time'] { -webkit-appearance: none; appearance: none; text-align: center; }";
  content += "input[type='time']::-webkit-calendar-picker-indicator { filter: invert(1); cursor: pointer; }";
  content += "input:focus, select:focus { border-color: var(--accent); outline: none; box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.2); }";
  content += "input:disabled { opacity: 0.5; cursor: not-allowed; background-color: #1e293b; }";
  content += "fieldset { border: 1px solid var(--border); border-radius: 8px; padding: 20px; margin-top: 15px; background: rgba(0,0,0,0.1); min-width: 0; overflow: hidden; }";
  content += "legend { color: var(--accent); font-weight: 700; padding: 0 10px; font-size: 0.9rem; text-transform: uppercase; }";

  // Animation Control Styles
  content += ".anim-label { margin-top: 20px; margin-bottom: 10px; font-weight: 600; display: block; }";
  content += ".anim-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-bottom: 15px; padding: 0 12px 16px; background: rgba(255,255,255,0.05); border-radius: 8px; border: 1px solid rgba(255,255,255,0.1); }";
  content += ".anim-item { cursor: pointer; font-size: 0.9em; display: flex; align-items: center; }";
  content += ".anim-item input { margin-right: 8px; }";

  // Draggable Screens List
  content += ".sortable-list { list-style: none; padding: 0; margin: 15px 0 0; border: 1px solid var(--border); border-radius: 8px; overflow: hidden; background: rgba(0,0,0,0.1); }";
  content += ".sortable-item { padding: 12px 15px; border-bottom: 1px solid var(--border); display: flex; align-items: center; cursor: grab; color: var(--text); background: var(--card); transition: background 0.2s; }";
  content += ".sortable-item:last-child { border-bottom: none; }";
  content += ".sortable-item:active { cursor: grabbing; }";
  content += ".sortable-item.disabled { opacity: 0.4; background: transparent; }";
  content += ".drag-handle { margin-right: 15px; color: var(--text-muted); font-size: 1.2rem; }";
  content += ".sortable-item.dragging { opacity: 0.5; background: rgba(59, 130, 246, 0.2); }";

  // Buttons & Misc
  content += "button { background-color: var(--accent); color: white; padding: 16px; border: none; border-radius: 8px; cursor: pointer; margin-top: 30px; width: 100%; font-size: 1.1rem; font-weight: 700; transition: opacity 0.2s; }";
  content += "button:hover { opacity: 0.9; }";
  content += ".no-data-tile { grid-column: 1 / -1; background: rgba(59, 130, 246, 0.05); border: 1px solid var(--accent); padding: 30px; color: var(--accent); border-radius: 10px; text-align: center; margin-top: 15px}";
  content += ".update-footer { text-align: center; font-size: 0.8rem; color: var(--text-muted); margin-top: 15px; font-family: monospace; }";
  content += ".collapsible { transition: all 0.3s ease; }";
  content += ".hidden { display: none; }";
  content += "hr { border: 0; border-top: 1px solid var(--border); margin: 25px 0; }";
  content += "@media (max-width: 600px) { .dashboard-grid { grid-template-columns: 1fr; } #time-display { font-size: 3.5rem; } }";
  content += "</style></head><body><div class='container'>";

  content += "<div class='app-header'>Tinytosh</div>";
  content += "<div class='panel header-panel'><div id='time-display'>" + getCurrentTimeShort(config.time_format) + "</div>"; 
  content += "<h2 id='location-info'>📍 " + config.city + " (" + config.timezone + ")</h1></div>"; 

  content += "<form method='get' action='/save'>";

  // Combined Global Settings & Location Panel
  content += "<div class='panel'><h3 style='margin-top:0; color: var(--accent);'>Global Settings</h3>";

  // Hardware/Interval Settings
  content += "<label>Data Sync Interval (Mins):</label><input type='number' name='refresh_min' value='" + String(config.refresh_interval_min) + "'>";
  
  // Auto Cycle Checkbox
  content += "<label style='margin: 10px 0 0; cursor:pointer; font-weight:600;'><input type='checkbox' id='autoCycle' name='auto_cycle' value='1' " + String(config.screen_auto_cycle ? "checked" : "") + "> Cycle Screens Automatically</label>";
  content += "<p style='font-size:0.8em; color:var(--text-muted); margin-top:8px;'>If disabled, screens will only change when you press the button.</p>";
  
  // Screen Cycle Interval
  content += "<label>Screen Cycle Interval (Secs):</label><input type='number' id='screenIntInput' name='screen_int' value='" + String(config.screen_interval_sec) + "'>";

  // Animation Checkboxes
  content += "<label class='anim-label'>Active Animations:</label>";
  content += "<p style='font-size:0.8em; color:var(--text-muted); margin-top:-5px; margin-bottom:10px;'>If 'None' is unchecked, select which animations to cycle through.</p>";
  
  content += "<div class='anim-grid'>";
  
  const char* animLabels[] = {
    "🚫 None",              // 0
    "↔️ Slide Horizontal",  // 1
    "↕️ Slide Vertical",    // 2
    "👾 Dissolve (Noise)",  // 3
    "🎭 Curtain Open",      // 4
    "🎹 Venetian Blinds"    // 5
  };

  content += "<input type='hidden' id='finalMask' name='anim_mask' value='" + String(config.anim_mask) + "'>";

  bool isNone = (config.anim_mask == 0);
  content += "<label class='anim-item'>"; 
  content += "<input type='checkbox' id='animNone' " + String(isNone ? "checked" : "") + ">";
  content += String(animLabels[0]);
  content += "</label>";

  for (int i = 1; i <= 5; i++) {
    bool isSet = (config.anim_mask & (1 << i));
    String checked = isSet ? "checked" : "";
    
    content += "<label class='anim-item'>";
    content += "<input type='checkbox' class='anim-chk' value='" + String(1 << i) + "' " + checked + ">";
    content += String(animLabels[i]);
    content += "</label>";
  }
  content += "</div>";

  // Time Format Radios
  content += "<label>Time Format:</label><div style='display:flex; gap:15px; margin-top:8px;'>";
  content += "<label style='cursor:pointer; display:flex; align-items:center;'><input type='radio' name='time_format' value='24' " + String(config.time_format == "24" ? "checked" : "") + " style='margin-right:6px;'> 24-Hour</label>";
  content += "<label style='cursor:pointer; display:flex; align-items:center;'><input type='radio' name='time_format' value='12' " + String(config.time_format == "12" ? "checked" : "") + " style='margin-right:6px;'> 12-Hour</label></div>";
  content += "<p style='font-size:0.8em; color:var(--text-muted); margin-top:8px;'>Format affects both the OLED display and the Web Panel.</p>";

  content += "<hr style='border: 0; border-top: 1px solid var(--border); margin: 25px 0;'>";

  // Location Sub-section
  content += "<label style='margin:0; cursor:pointer; font-weight:600;'><input type='checkbox' id='autoDetect' name='auto_detect' value='1' " + String(config.auto_detect ? "checked" : "") + "> Detect Location Automatically (IP)</label>";
  content += "<p style='font-size:0.85em; color: var(--text-muted); margin: 5px 0 15px 25px;'>Uses your IP address to determine city, coordinates, and timezone.</p>";

  content += "<fieldset id='manualFields' class='collapsible' style='border: 1px solid var(--border); border-radius: 8px; padding: 15px; margin-top: 10px;'>";
  content += "<legend style='font-weight: 600; color: var(--accent); padding: 0 10px;'>Manual Location Entry</legend>";

  content += "<label>City Name:</label><input type='text' name='city' value='" + config.city + "'>";

  content += "<div class='dashboard-grid' style='margin-top: 0;'>";
  content += "  <div><label style='margin-top: 0;'>Latitude:</label><input type='number' step='any' name='latitude' value='" + String(config.latitude, 4) + "'></div>";
  content += "  <div><label style='margin-top: 0;'>Longitude:</label><input type='number' step='any' name='longitude' value='" + String(config.longitude, 4) + "'></div>";
  content += "</div>";

  content += "<label>Timezone:</label><select name='timezone' style='width:100%;'>";
  DynamicJsonDocument tzDoc(6144); 
  deserializeJson(tzDoc, POSIX_TIMEZONE_MAP); 
  for (JsonPair p : tzDoc.as<JsonObject>()) {
      String key = p.key().c_str();
      content += "<option value='" + key + "'" + (key == config.timezone ? " selected" : "") + ">" + key + "</option>";
  }
  content += "</select></fieldset>";

  content += "<hr style='border: 0; border-top: 1px solid var(--border); margin: 25px 0;'>";
  content += "<label style='margin:0; cursor:pointer; font-weight:600;'><input type='checkbox' id='nightMode' name='night_mode' value='1' " + String(config.night_mode ? "checked" : "") + "> Enable Night Mode</label>";
  content += "<p style='font-size:0.85em; color: var(--text-muted); margin: 5px 0 15px 25px;'>Set a quiet schedule to pause animations, dim the screen, or save API calls.</p>";

  content += "<fieldset id='nightFields' class='collapsible' style='border: 1px solid var(--border); border-radius: 8px; padding: 15px; margin-top: 10px;'>";
  content += "<legend style='font-weight: 600; color: var(--accent); padding: 0 10px;'>Night Schedule</legend>";

  content += "<div class='dashboard-grid' style='margin-top: 0;'>";
  content += "  <div><label style='margin-top: 0;'>Start Time:</label><input type='time' name='night_start' value='" + String(config.night_start) + "'></div>";
  content += "  <div><label style='margin-top: 0;'>End Time:</label><input type='time' name='night_end' value='" + String(config.night_end) + "'></div>";
  content += "</div>";
  
  content += "<label>Screen Action:</label><select name='night_action' style='width:100%;'>";
  content += "<option value='0' " + String(config.night_action == 0 ? "selected" : "") + ">No Visual Change</option>";
  content += "<option value='1' " + String(config.night_action == 1 ? "selected" : "") + ">Dim Display</option>";
  content += "<option value='2' " + String(config.night_action == 2 ? "selected" : "") + ">Turn Display Off</option>";
  content += "</select></fieldset></div>";

  // Screen Display Order Panel
  content += "<div class='panel'><h3 style='margin-top:0; color: var(--accent);'>Screen Display Order</h3>";
  content += "<p style='font-size:0.85em; color:var(--text-muted); margin-top:-5px;'>Drag and drop to rearrange. Disabled screens are locked at the bottom.</p>";
  
  content += "<ul id='sortable-list' class='sortable-list'>";
  
  for (int i = 0; i < NUM_SCREENS; i++) {
    int screenId = config.screen_order[i];
    
    String targetId = "";
    switch(screenId) {
      case SCREEN_TIME: targetId = "showTime"; break;
      case SCREEN_WEATHER: targetId = "showWeather"; break;
      case SCREEN_AIR_QUALITY: targetId = "showAQI"; break;
      case SCREEN_CRYPTO: targetId = "showCrypto"; break;
      case SCREEN_CURRENCY: targetId = "showCurrency"; break;
      case SCREEN_STOCK: targetId = "showStock"; break;
      case SCREEN_PC_MONITOR: targetId = "showPc"; break;
    }
    
    content += "<li class='sortable-item' data-id='" + String(screenId) + "' data-target='" + targetId + "' draggable='true'>";
    content += "<span class='drag-handle'>☰</span>";
    content += String(SCREEN_NAMES[screenId]);
    content += "</li>";
  }
  content += "</ul>";
  content += "<input type='hidden' name='screen_order' id='screenOrderInput' value=''>";
  content += "</div>";

  // Dynamic Screen Panels
  for (int i = 0; i < NUM_SCREENS; i++) {
      int screenId = config.screen_order[i];

      switch (screenId) {
          case SCREEN_TIME: {
              content += "<div class='panel'>";
              content += "<label style='margin:0; cursor:pointer;'><input type='checkbox' id='showTime' name='show_time' value='1' " + String(config.show_time ? "checked" : "") + "> Time Screen</label>";

              content += "<div id='timeContent' class='collapsible'>";
              content += "<div class='dashboard-grid'>";

              content += "<div class='tile'>";
              content += "<div class='tile-icon'>🕒</div>";
              content += "<div class='tile-value' id='preview-time'>" + getCurrentTimeShort(config.time_format) + "</div>";
              content += "<div class='tile-label'>Current Time</div>";
              content += "</div>";

              content += "<div class='tile'>";
              content += "<div class='tile-icon'>📅</div>";
              content += "<div class='tile-value' id='preview-date' style='font-size: 1.2rem;'>" + getFullDate() + "</div>";
              content += "<div class='tile-label'>Current Date</div>";
              content += "</div></div>";

              content += "<label style='margin-top:10px; cursor:pointer;'><input type='checkbox' name='date_display' value='1' " + String(config.date_display ? "checked" : "") + "> Display Date</label>";
              content += "</div></div>";
              break;
          }
          
          case SCREEN_WEATHER: {
              content += "<div class='panel'>";
              content += "<label style='margin:0; cursor:pointer;'><input type='checkbox' id='showWeather' name='show_weather' value='1' " + String(config.show_weather ? "checked" : "") + "> Weather Screen</label>";
              content += "<div id='weatherContent' class='collapsible'>";
              if (!weatherValid) {
                  content += "<div class='no-data-tile'>☁️ Weather data will be available after sync</div>";
              } else {
                  content += "<div class='dashboard-grid'>";
                  content += "<div class='tile'><div class='tile-icon' id='icon-temp'>" + getWeatherIcon(weather.weather_code) + "</div><div class='tile-value' id='value-temp'>" + String(weather.temp, 1) + " °" + config.temp_unit + "</div><div class='tile-label'>Temperature</div></div>";
                  content += "<div class='tile'><div class='tile-icon'>🤒</div><div class='tile-value' id='value-feels'>" + String(weather.apparent_temperature, 1) + " °" + config.temp_unit + "</div><div class='tile-label'>Feels Like</div></div>";
                  content += "<div class='tile'><div class='tile-icon'>💧</div><div class='tile-value' id='value-hum'>" + String(weather.humidity) + "%</div><div class='tile-label'>Humidity</div></div>";
                  content += "<div class='tile'><div class='tile-icon'>💨</div><div class='tile-value' id='value-wind'>" + String(weather.wind_speed, 1) + " km/h</div><div class='tile-label'>Wind Speed</div></div>";
                  content += "</div>";
                  content += "<div class='update-footer' id='weather-upd'>Last Update: " + weather.update_time + "</div>";
              }
              
              content += "<label>Temperature Unit:</label><div style='display:flex; gap:15px; margin-top:8px;'>";
              content += "<label style='cursor:pointer; display:flex; align-items:center;'><input type='radio' name='temp_unit' value='C' " + String(config.temp_unit == "C" ? "checked" : "") + " style='margin-right:6px;'> °C</label>";
              content += "<label style='cursor:pointer; display:flex; align-items:center;'><input type='radio' name='temp_unit' value='F' " + String(config.temp_unit == "F" ? "checked" : "") + " style='margin-right:6px;'> °F</label></div>";
              
              content += "<label style='margin-top:10px; cursor:pointer;'><input type='checkbox' name='round_temps' value='1' " + String(config.round_temps ? "checked" : "") + "> Round Temperature Values</label>";
              content += "</div></div>";
              break;
          }

          case SCREEN_AIR_QUALITY: {
              content += "<div class='panel'>";
              content += "<label style='margin:0; cursor:pointer;'><input type='checkbox' id='showAQI' name='show_aqi' value='1' " + String(config.show_aqi ? "checked" : "") + "> Air Quality Screen</label>";
              content += "<div id='aqiContent' class='collapsible'>";

              if (!aqiValid) {
                  content += "<div class='no-data-tile'>🍃 Air quality data will be available after sync</div>";
              } else {
                  content += "<div class='dashboard-grid'>";
                  content += "<div class='tile'><div class='tile-icon'>🍃</div><div class='tile-value' id='value-aqi'>" + String(aqi.aqi) + "</div><div class='tile-label'>" + aqi.status + " Index</div></div>";
                  content += "<div class='tile'><div class='tile-icon'>🌫️</div><div class='tile-value' id='value-pm25'>" + String(aqi.pm25, 1) + " <small>µg</small></div><div class='tile-label'>PM 2.5</div></div>";
                  content += "<div class='tile'><div class='tile-icon'>🏭</div><div class='tile-value' id='value-pm10'>" + String(aqi.pm10, 1) + " <small>µg</small></div><div class='tile-label'>PM 10</div></div>";
                  content += "<div class='tile'><div class='tile-icon'>🧪</div><div class='tile-value' id='value-no2'>" + String(aqi.no2, 1) + " <small>µg</small></div><div class='tile-label'>Nitrogen Dioxide</div></div>";
                  content += "</div>";
                  content += "<div class='update-footer' id='aqi-upd'>Last Update: " + weather.update_time + "</div>";
              }

              content += "<label>AQI Standard:</label><div style='display:flex; gap:15px; margin-top:8px;'>";
              content += "<label style='cursor:pointer; display:flex; align-items:center;'><input type='radio' name='aqi_type' value='US' " + String(config.aqi_type == "US" ? "checked" : "") + " style='margin-right:6px;'> US Standard</label>";
              content += "<label style='cursor:pointer; display:flex; align-items:center;'><input type='radio' name='aqi_type' value='EU' " + String(config.aqi_type == "EU" ? "checked" : "") + " style='margin-right:6px;'> European Standard</label></div>";
              content += "<p style='font-size:0.8em; color:#888; margin-top:8px;'>EU: 0-100+ scale | US: 0-500 scale</p>";

              content += "</div></div>";
              break;
          }

          case SCREEN_STOCK: {
              content += "<div class='panel'>";
              content += "<label style='margin:0; cursor:pointer;'><input type='checkbox' id='showStock' name='show_stock' value='1' " + String(config.show_stock ? "checked" : "") + "> Stock Tracking Screen</label>";
              content += "<div id='stockContent' class='collapsible'>";
              
              if (!stockValid) {
                  content += "<div class='no-data-tile'>📈 Stock data will be available after sync</div>";
              } else {
                  content += "<div class='dashboard-grid'>";
                  content += "<div class='tile'><div class='tile-icon'>📊</div><div class='tile-value' id='stock-price'>$" + String(stock.price, 2) + "</div><div class='tile-label' id='stock-sym'>" + stock.symbol + " Price</div></div>";
                  content += "<div class='tile'><div class='tile-icon' id='stock-trend-icon'>" + String(stock.percent_change >= 0 ? "📈" : "📉") + "</div><div class='tile-value' id='stock-change'>" + String(stock.percent_change, 2) + "%</div><div class='tile-label'>Daily Change</div></div>";
                  content += "</div>";
                  content += "<div class='update-footer' id='stock-upd'>Last Update: " + weather.update_time + "</div>";
              }

              content += "<label>Track Stock/ETF:</label><select name='stock_symbol'>";
              for(auto s : topStocks) {
                  content += "<option value='" + String(s.ticker) + "' " + 
                             (String(config.stock_symbol) == String(s.ticker) ? "selected" : "") + 
                             ">" + String(s.name) + " - " + String(s.ticker) + "</option>";
              }
              content += "</select>";
              content += "<label style='margin-top:10px; cursor:pointer;'><input type='checkbox' name='stock_fn' value='1' " + String(config.stock_fn ? "checked" : "") + "> Display Full Company Name</label>";
              content += "</div></div>";
              break;
          }

          case SCREEN_CRYPTO: {
              content += "<div class='panel'>";
              content += "<label style='margin:0; cursor:pointer;'><input type='checkbox' id='showCrypto' name='show_crypto' value='1' " + String(config.show_crypto ? "checked" : "") + "> Crypto Tracking Screen</label>";
              content += "<div id='cryptoContent' class='collapsible'>";
              if (!cryptoValid) {
                  content += "<div class='no-data-tile'>💰 Crypto data will be available after sync</div>";
              } else {
                  content += "<div class='dashboard-grid'>";
                  content += "<div class='tile'><div class='tile-icon'>₿</div><div class='tile-value' id='crypto-price'>" + String((int)round(crypto.price_usd)) + "$</div><div class='tile-label' id='crypto-sym'>" + crypto.symbol + " Price</div></div>";
                  content += "<div class='tile'><div class='tile-icon' id='crypto-trend-icon'>" + String(crypto.percent_change_24h >= 0 ? "📈" : "📉") + "</div><div class='tile-value' id='crypto-change'>" + String(crypto.percent_change_24h, 1) + "%</div><div class='tile-label'>24h Change</div></div>";
                  content += "</div>";
                  content += "<div class='update-footer' id='crypto-upd'>Last Update: " + weather.update_time + "</div>";
              }
              content += "<label>Track Cryptocurrency:</label><select name='crypto_id'>";
              for(auto coin : topCoins) {
                  content += "<option value='" + String(coin.id) + "' " + (config.crypto_id == coin.id ? "selected" : "") + ">" + String(coin.sym) + "</option>";
              }
              content += "</select>";
              content += "<label style='margin-top:10px; cursor:pointer;'><input type='checkbox' name='crypto_fn' value='1' " + String(config.crypto_fn ? "checked" : "") + "> Display Full Coin Name</label>";
              content += "</div></div>";
              break;
          }

          case SCREEN_CURRENCY: {
              content += "<div class='panel'>";
              content += "<label style='margin:0; cursor:pointer;'><input type='checkbox' id='showCurrency' name='show_currency' value='1' " + String(config.show_currency ? "checked" : "") + "> Currency Exchange Screen</label>";
              content += "<div id='currencyContent' class='collapsible'>";
              
              if (!currencyValid) {
                  content += "<div class='no-data-tile'>💱 Currency data will be available after sync</div>";
              } else {
                  content += "<div class='dashboard-grid'>";
                  
                  float displayRate = currency.rate * config.currency_multiplier;
                  int decimals = 0;
                  if (displayRate < 10.0) decimals = 3;
                  else if (displayRate < 100.0) decimals = 2;
                  else if (displayRate < 1000.0) decimals = 1;

                  content += "<div class='tile'><div class='tile-icon'>💵</div><div class='tile-value' id='currency-base-val'>" + String(config.currency_multiplier) + " " + currency.base + "</div><div class='tile-label'>Base Amount</div></div>";
                  content += "<div class='tile'><div class='tile-icon'>💱</div><div class='tile-value' id='currency-target-val'>" + String(displayRate, decimals) + " " + currency.target + "</div><div class='tile-label'>Exchange Rate</div></div>";
                  
                  content += "</div>";
                  content += "<div class='update-footer' id='currency-upd'>Last Update: " + weather.update_time + "</div>";
              }

              content += "<div class='dashboard-grid' style='margin-top: 10px;'>";
              
              content += "<div><label style='margin-top: 0;'>Base Currency:</label><select name='currency_base'>";
              for (auto c : allCurrencies) {
                  String codeUpper = String(c.code);
                  codeUpper.toUpperCase();
                  content += "<option value='" + String(c.code) + "' " + (String(config.currency_base) == String(c.code) ? "selected" : "") + ">" + codeUpper + " - " + String(c.name) + "</option>";
              }
              content += "</select></div>";

              content += "<div><label style='margin-top: 0;'>Target Currency:</label><select name='currency_target'>";
              for (auto c : allCurrencies) {
                  String codeUpper = String(c.code);
                  codeUpper.toUpperCase();
                  content += "<option value='" + String(c.code) + "' " + (String(config.currency_target) == String(c.code) ? "selected" : "") + ">" + codeUpper + " - " + String(c.name) + "</option>";
              }
              content += "</select></div>";
              
              content += "</div>";

              content += "<label>Multiplier Amount:</label><select name='currency_multiplier'>";
              int multipliers[] = {1, 10, 100, 1000, 10000, 100000};
              for (int m : multipliers) {
                  content += "<option value='" + String(m) + "' " + (config.currency_multiplier == m ? "selected" : "") + ">" + String(m) + "</option>";
              }
              content += "</select>";
              content += "<label style='margin-top:10px; cursor:pointer;'><input type='checkbox' name='currency_fn' value='1' " + String(config.currency_fn ? "checked" : "") + "> Display Full Currency Name</label>";
              content += "</div></div>";
              break;
          }

          case SCREEN_PC_MONITOR: {
              content += "<div class='panel'>";
              content += "<label style='margin:0; cursor:pointer;'><input type='checkbox' id='showPc' name='show_pc' value='1' " + String(config.show_pc ? "checked" : "") + "> PC Monitoring Screen</label>";
              content += "<div id='pcContent' class='collapsible'>";
              if (!pcValid) {
                  content += "<div class='no-data-tile'>🖥️ PC data will be available after sync</div>";
              } else {
                  content += "<div class='dashboard-grid'>";
                  content += "<div class='tile'><div class='tile-icon'>📊</div><div class='tile-value' id='pc-cpu'>" + String((int)round(pc.cpu_percent)) + "%</div><div class='tile-label'>CPU Usage</div></div>";
                  content += "<div class='tile'><div class='tile-icon'>🧠</div><div class='tile-value' id='pc-ram'>" + String((int)round(pc.mem_percent)) + "%</div><div class='tile-label'>RAM Usage</div></div>";
                  content += "<div class='tile'><div class='tile-icon'>💽</div><div class='tile-value' id='pc-disk'>" + String((int)round(pc.disk_percent)) + "%</div><div class='tile-label'>Disk Usage</div></div>";
                  content += "<div class='tile'><div class='tile-icon'>⬇️</div><div class='tile-value' id='pc-net'>" + String((int)round(pc.net_down_kb)) + " KB/s</div><div class='tile-label'>Download</div></div>";      
                  content += "</div>";
              }
              content += "</div></div>";
              break;
          }
      }
  }

  content += "<button type='submit'>💾 Save & Apply All Settings</button></form>";
  
  content += "<script>";
  content += "function updateVisibility(){";
  
  content += "  var pairs = [['autoDetect','manualFields',true], ['nightMode','nightFields',false], ['showTime', 'timeContent',false], ['showWeather','weatherContent',false], ['showPc','pcContent',false], ['showCrypto','cryptoContent',false], ['showCurrency','currencyContent',false], ['showStock','stockContent',false], ['showAQI','aqiContent',false]];";
  content += "  pairs.forEach(p => {";
  content += "    var ch = document.getElementById(p[0]); if(!ch) return;";
  content += "    var target = document.getElementById(p[1]);";
  content += "    var shouldHide = p[2] ? ch.checked : !ch.checked;";
  content += "    target.className = shouldHide ? 'collapsible hidden' : 'collapsible';";
  content += "    target.querySelectorAll('input, select').forEach(el => el.disabled = shouldHide);";
  content += "  });";

  content += "  var ac = document.getElementById('autoCycle');";
  content += "  var si = document.getElementById('screenIntInput');";
  content += "  if(ac && si) si.disabled = !ac.checked;";

  content += "}";
  
  content += "['autoDetect', 'nightMode', 'showTime', 'showWeather', 'showPc', 'showCrypto', 'showCurrency', 'showStock', 'showAQI', 'autoCycle'].forEach(id => { var el=document.getElementById(id); if(el) el.addEventListener('change', updateVisibility); });";
  content += "updateVisibility();";

  // Handle "None" Checkbox Logic
  content += "function toggleNone() {";
  content += "  const noneBox = document.getElementById('animNone');";
  content += "  const others = document.querySelectorAll('.anim-chk');";
  
  content += "  others.forEach(cb => {";
  content += "    cb.disabled = noneBox.checked;";
  content += "    if(noneBox.checked) cb.checked = false;";
  content += "    cb.parentElement.style.opacity = noneBox.checked ? '0.5' : '1';";
  content += "  });";
  content += "}";

  content += "function checkSafetyNet() {";
  content += "  if(!document.getElementById('animNone').checked) {";
  content += "    let count = 0;";
  content += "    document.querySelectorAll('.anim-chk').forEach(cb => { if(cb.checked) count++; });";
  content += "    if(count === 0) {";
  content += "      document.getElementById('animNone').checked = true;";
  content += "      toggleNone();";
  content += "    }";
  content += "  }";
  content += "}";

  content += "const nb = document.getElementById('animNone');";
  content += "if(nb) nb.addEventListener('change', toggleNone);";

  content += "document.querySelectorAll('.anim-chk').forEach(cb => {";
  content += "  cb.addEventListener('change', checkSafetyNet);";
  content += "});";
  
  content += "toggleNone();";

  content += "const list = document.getElementById('sortable-list');";
  content += "const orderInput = document.getElementById('screenOrderInput');";

  // Function to sync DOM list with checkbox states using the data-target attribute
  content += "function syncScreenOrder() {";
  content += "  const items = [...list.querySelectorAll('.sortable-item')];";
  content += "  let enabled = [], disabled = [];";
  content += "  items.forEach(item => {";
  content += "    const targetId = item.getAttribute('data-target');";
  content += "    const cb = document.getElementById(targetId);";
  content += "    if (cb && cb.checked) {";
  content += "      item.classList.remove('disabled'); item.setAttribute('draggable', 'true'); enabled.push(item);";
  content += "    } else {";
  content += "      item.classList.add('disabled'); item.removeAttribute('draggable'); disabled.push(item);";
  content += "    }";
  content += "  });";
  content += "  list.innerHTML = '';";
  content += "  enabled.forEach(el => list.appendChild(el));"; 
  content += "  disabled.forEach(el => list.appendChild(el));"; 
  content += "  updateOrderValue();";
  content += "}";

  content += "function updateOrderValue() {";
  content += "  const items = [...list.querySelectorAll('.sortable-item')];";
  content += "  orderInput.value = items.map(item => item.getAttribute('data-id')).join(',');";
  content += "}";

  // Hook Checkboxes to the sync function
  content += "const panelCheckboxes = ['showTime', 'showWeather', 'showAQI', 'showCrypto', 'showCurrency', 'showStock', 'showPc'];";
  content += "panelCheckboxes.forEach(id => {";
  content += "  const el = document.getElementById(id);";
  content += "  if (el) el.addEventListener('change', syncScreenOrder);";
  content += "});";

  // Universal Drag & Touch Logic
  content += "function getDragAfterEl(y) {";
  content += "  return [...list.querySelectorAll('.sortable-item:not(.dragging):not(.disabled)')].reduce((closest, child) => {";
  content += "    const box = child.getBoundingClientRect();";
  content += "    const offset = y - box.top - box.height / 2;";
  content += "    if (offset < 0 && offset > closest.offset) return { offset: offset, element: child };";
  content += "    else return closest;";
  content += "  }, { offset: Number.NEGATIVE_INFINITY }).element;";
  content += "}";

  content += "function moveItem(y) {";
  content += "  const draggable = document.querySelector('.dragging');";
  content += "  if (!draggable) return;";
  content += "  const afterEl = getDragAfterEl(y);";
  content += "  if (afterEl == null) {";
  content += "    const firstDis = list.querySelector('.disabled');";
  content += "    if (firstDis) list.insertBefore(draggable, firstDis);";
  content += "    else list.appendChild(draggable);";
  content += "  } else { list.insertBefore(draggable, afterEl); }";
  content += "}";

  // Standard Mouse Events (PC)
  content += "list.addEventListener('dragstart', e => { if (e.target.classList.contains('disabled')) { e.preventDefault(); return; } e.target.classList.add('dragging'); });";
  content += "list.addEventListener('dragend', e => { e.target.classList.remove('dragging'); updateOrderValue(); });";
  content += "list.addEventListener('dragover', e => { e.preventDefault(); moveItem(e.clientY); });";

  // Touch Events (Mobile)
  content += "list.addEventListener('touchstart', e => {";
  content += "  const item = e.target.closest('.sortable-item');";
  content += "  if (!item || item.classList.contains('disabled')) return;";
  content += "  item.classList.add('dragging');";
  content += "}, {passive: false});";

  content += "list.addEventListener('touchmove', e => {";
  content += "  if (!document.querySelector('.dragging')) return;";
  content += "  e.preventDefault();"; // Stops the whole page from scrolling while you drag the item
  content += "  moveItem(e.touches[0].clientY);";
  content += "}, {passive: false});";

  content += "list.addEventListener('touchend', e => {";
  content += "  const dragging = document.querySelector('.dragging');";
  content += "  if (dragging) { dragging.classList.remove('dragging'); updateOrderValue(); }";
  content += "});";

  content += "syncScreenOrder();";
  
  // Mask Calculator
  content += "document.querySelector('form').addEventListener('submit', function(e) {";
  content += "  let mask = 0;";
  content += "  document.querySelectorAll('.anim-chk').forEach(cb => {";
  content += "    if(cb.checked) mask += parseInt(cb.value);";
  content += "  });";
  content += "  document.getElementById('finalMask').value = mask;";
  content += "});";
  
  content += "function updateData() { fetch('/update').then(r => r.json()).then(d => {";
  content += "  const set = (id, val, html=false) => { const el = document.getElementById(id); if(el) { if(html) el.innerHTML = val; else el.innerText = val; }};";

  content += "  set('time-display', d.time);"; 
  content += "  set('preview-time', d.time);";
  content += "  set('preview-date', d.date);";

  content += "  if (d.temp !== undefined) {";
  content += "    set('value-temp', d.temp + ' °' + d.temp_unit);";
  content += "    set('value-feels', d.apparent_temperature + ' °' + d.temp_unit);";
  content += "    set('value-hum', d.humidity + '%');";
  content += "    set('value-wind', d.wind_speed + ' km/h');";
  content += "    set('weather-upd', 'Last Update: ' + d.update_time);";
  content += "  }";

  content += "  if (d.aqi !== undefined && d.aqi !== -1) {";
  content += "    set('value-aqi', d.aqi);";
  content += "    const aqiLabel = document.querySelector('#value-aqi + .tile-label'); if(aqiLabel) aqiLabel.innerText = d.aqi_status + ' Index';"; 
  content += "    set('value-pm25', d.pm25 + ' <small>µg</small>', true);";
  content += "    set('value-pm10', d.pm10 + ' <small>µg</small>', true);";
  content += "    set('value-no2', d.no2 + ' <small>µg</small>', true);";
  content += "    set('aqi-upd', 'Last Update: ' + d.update_time);";
  content += "  }";

  content += "  if (d.pc_cpu !== undefined) {";
  content += "    set('pc-cpu', Math.round(d.pc_cpu) + '%');";
  content += "    set('pc-net', Math.round(d.pc_net) + ' KB/s');";
  content += "    set('pc-ram', Math.round(d.pc_ram) + '%');";
  content += "    set('pc-disk', Math.round(d.pc_disk) + '%');";
  content += "  }";

  content += "  if (d.crypto_price !== undefined) {";
  content += "    set('crypto-price', d.crypto_price + '$');";
  content += "    set('crypto-change', d.crypto_change + '%');";
  content += "    set('crypto-trend-icon', d.crypto_change >= 0 ? '📈' : '📉');";
  content += "    set('crypto-upd', 'Last Update: ' + d.update_time);";
  content += "  }";

  content += "  if (d.currency_base_text !== undefined) {";
  content += "    set('currency-base-val', d.currency_base_text);";
  content += "    set('currency-target-val', d.currency_target_text);";
  content += "    set('currency-upd', 'Last Update: ' + d.update_time);";
  content += "  }";
  
  content += "  if (d.stock_price !== undefined) {";
  content += "    set('stock-price', '$' + d.stock_price);";
  content += "    set('stock-change', d.stock_change + '%');";
  content += "    set('stock-trend-icon', d.stock_change >= 0 ? '📈' : '📉');";
  content += "    set('stock-sym', d.stock_symbol + ' Price');"; 
  content += "    set('stock-upd', 'Last Update: ' + d.update_time);";
  content += "  }";
    
  content += "}).catch(e => console.log('Sync error:', e)); } setInterval(updateData, 15000); updateData();";
  content += "</script></div></body></html>";

  return content;
}

void WebServerService::handleRoot() {
  server.send(200, "text/html", generateRootPageContent());
}

void WebServerService::handleSave() {
  Config& config = state->config;
  WeatherData& weather = state->weather;
  AirQualityData& aqi = state->aqi;
  CryptoData& crypto = state->crypto;
  CurrencyData& currency = state->currency;
  StockData& stock = state->stock;
  PcStats& pc = state->pc;

  // 1. Update Screen Visibility & Master Toggles
  config.auto_detect = server.hasArg("auto_detect");
  config.screen_auto_cycle = server.hasArg("auto_cycle");
  config.night_mode = server.hasArg("night_mode");

  config.show_time = server.hasArg("show_time");
  config.show_weather = server.hasArg("show_weather");
  config.show_aqi = server.hasArg("show_aqi");
  config.show_crypto = server.hasArg("show_crypto");
  config.show_pc = server.hasArg("show_pc");
  config.show_currency = server.hasArg("show_currency");
  config.show_stock = server.hasArg("show_stock");

  if (server.hasArg("screen_order")) {
    String orderStr = server.arg("screen_order");
    int idx = 0;
    int startPos = 0;
    
    while (startPos < orderStr.length() && idx < NUM_SCREENS) {
      int commaPos = orderStr.indexOf(',', startPos);
      if (commaPos == -1) {
        config.screen_order[idx++] = orderStr.substring(startPos).toInt();
        break;
      } else {
        config.screen_order[idx++] = orderStr.substring(startPos, commaPos).toInt();
        startPos = commaPos + 1;
      }
    }
  }

  if (config.show_time) config.date_display = server.hasArg("date_display");
  if (config.show_weather) config.round_temps = server.hasArg("round_temps");

  if (config.show_crypto) config.crypto_fn = server.hasArg("crypto_fn");
  if (config.show_currency) config.currency_fn = server.hasArg("currency_fn");
  if (config.show_stock) config.stock_fn = server.hasArg("stock_fn");

  // 2. Persistent Settings: Only update if the arg is present 
  if (server.hasArg("time_format")) config.time_format = server.arg("time_format");
  if (server.hasArg("temp_unit")) config.temp_unit = server.arg("temp_unit");
  if (server.hasArg("aqi_type")) config.aqi_type = server.arg("aqi_type");
  if (server.hasArg("refresh_min")) config.refresh_interval_min = server.arg("refresh_min").toInt();
  if (server.hasArg("screen_int")) config.screen_interval_sec = server.arg("screen_int").toInt();
  if (server.hasArg("anim_mask")) config.anim_mask = server.arg("anim_mask").toInt();
  if (server.hasArg("crypto_id")) config.crypto_id = server.arg("crypto_id").toInt();

  // Night Mode Settings
  if (server.hasArg("night_action")) config.night_action = server.arg("night_action").toInt();
  if (server.hasArg("night_start")) config.night_start = server.arg("night_start");
  if (server.hasArg("night_end")) config.night_end = server.arg("night_end");

  // Currency Settings
  if (server.hasArg("currency_base")) {
      config.currency_base = server.arg("currency_base"); 
  }
  if (server.hasArg("currency_target")) {
      config.currency_target = server.arg("currency_target");
  }
  if (server.hasArg("currency_multiplier")) {
      config.currency_multiplier = server.arg("currency_multiplier").toInt();
  }

  // Stock Settings
  if (server.hasArg("stock_symbol")) {
      config.stock_symbol = server.arg("stock_symbol");
  }

  if (!config.auto_detect && server.hasArg("city")) {
    config.city = server.arg("city"); 
    config.latitude = server.arg("latitude").toFloat();
    config.longitude = server.arg("longitude").toFloat();
    config.timezone = server.arg("timezone");
  }

  // 3. Forcible Reset of Data (State clearing)
  if (!config.show_weather) {
    weather.temp = NAN;
    weather.humidity = NAN;
    weather.apparent_temperature = NAN;
    weather.wind_speed = NAN;
  }

  if (!config.show_aqi) {
    aqi.aqi = NAN;
    aqi.pm25 = NAN;
    aqi.pm10 = NAN;
    aqi.no2 = NAN;
  }

  if (!config.show_pc) {
    pc.cpu_percent = 0;
    pc.net_down_kb = 0;
    pc.mem_percent = 0;
    pc.disk_percent = 0;
  }

  if (!config.show_crypto) {
    crypto.price_usd = NAN;
    crypto.percent_change_24h = NAN;
  }

  if (!config.show_currency) {
    currency.rate = NAN;
    currency.updated = false;
  }

  if (!config.show_stock) {
    stock.price = NAN;
    stock.percent_change = NAN;
    stock.updated = false;
  }

  if (config.refresh_interval_min <= 0) config.refresh_interval_min = 1; 

  if (saveCallback) {
    saveCallback();
  }
  
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "Settings saved. Redirecting...");
}

void WebServerService::handleUpdate() {
  Config& config = state->config;
  WeatherData& weather = state->weather;
  AirQualityData& aqi = state->aqi;
  CryptoData& crypto = state->crypto;
  CurrencyData& currency = state->currency;
  StockData& stock = state->stock;
  PcStats& pc = state->pc;

  DynamicJsonDocument doc(1024); 
  
  doc["time"] = getCurrentTimeShort(config.time_format);
  doc["date"] = getFullDate();
  
  doc["update_time"] = weather.update_time;
  doc["temp"] = String(weather.temp, 1);
  doc["apparent_temperature"] = String(weather.apparent_temperature, 1);
  doc["humidity"] = String(weather.humidity);
  doc["wind_speed"] = String(weather.wind_speed, 1);
  doc["weather_code"] = weather.weather_code;
  doc["temp_unit"] = config.temp_unit;
  doc["time_format"] = config.time_format;

  doc["aqi"] = String(aqi.aqi);
  doc["aqi_status"] = aqi.status;
  doc["pm25"] = String(aqi.pm25, 1);
  doc["pm10"] = String(aqi.pm10, 1);
  doc["no2"] = String(aqi.no2, 1);

  doc["pc_cpu"] = String(pc.cpu_percent);
  doc["pc_net"] = String(pc.net_down_kb);
  doc["pc_ram"] = String(pc.mem_percent);
  doc["pc_disk"] = String(pc.disk_percent);

  doc["crypto_symbol"] = String(crypto.symbol);
  doc["crypto_price"] = String(crypto.price_usd);
  doc["crypto_change"] = String(crypto.percent_change_24h);
  
  if (currency.updated) {
    float displayRate = currency.rate * config.currency_multiplier;
    
    int decimals = 0;
    if (displayRate < 10.0) decimals = 3;
    else if (displayRate < 100.0) decimals = 2;
    else if (displayRate < 1000.0) decimals = 1;
    
    doc["currency_base_text"] = String(config.currency_multiplier) + " " + currency.base;
    doc["currency_target_text"] = String(displayRate, decimals) + " " + currency.target;
    doc["currency_date"] = currency.date;
  }
  
  if (stock.updated) {
      doc["stock_symbol"] = stock.symbol;
      doc["stock_price"] = String(stock.price, 2);
      doc["stock_change"] = String(stock.percent_change, 2);
  }
  
  String jsonResponse;
  serializeJson(doc, jsonResponse);
  
  server.send(200, "application/json", jsonResponse);
}