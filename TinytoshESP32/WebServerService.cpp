#include "WebServerService.h"
#include <ArduinoJson.h>
#include <ESPmDNS.h>
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
  server.on("/pc-stats", HTTP_POST, [this](){ this->handlePcStats(); });
  
  server.begin();
  Serial.println("WebServerService: HTTP Server started."); 
  
  String uniqueName = state->config.device_id;

  if (MDNS.begin(uniqueName.c_str())) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("WebServerService: mDNS Responder Started: http://%s.local\n", uniqueName.c_str());
  }
}

void WebServerService::handleClient() {
    server.handleClient();
}

void WebServerService::handleRoot() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(HTTP_OK, "text/html", ""); 

  String chunk;
  chunk.reserve(4096);
  auto add = [&](const String& str) {
    chunk += str;
    if (chunk.length() > 2048) {
      server.sendContent(chunk);
      chunk = "";
    }
  };

  Config& config = state->config;
  WeatherData& weather = state->weather;
  AirQualityData& aqi = state->aqi;
  CryptoData& crypto = state->crypto;
  CurrencyData& currency = state->currency;
  StockData& stock = state->stock;
  PcStats& pc = state->pc;
  PcMedia& media = state->media;
  
  bool weatherValid = !isnan(weather.temp);
  bool aqiValid = !isnan(aqi.pm25) && !isnan(aqi.pm10) && !isnan(aqi.no2);
  bool pcValid = pc.cpu_percent > 0.1; 
  bool cryptoValid = !isnan(crypto.price_usd) && crypto.price_usd > 0;
  bool currencyValid = currency.updated;
  bool stockValid = stock.updated;

  add("<html><head><title>Tinytosh | Web Panel</title>");
  add("<meta name='viewport' content='width=device-width, initial-scale=1'><meta charset='UTF-8'>");
  add("<style>");
  add(":root { --bg: #0f172a; --card: #1e293b; --accent: #3b82f6; --text: #f1f5f9; --text-muted: #94a3b8; --border: #334155; }");
  add("* { box-sizing: border-box; }");
  add("html, body { margin: 0; padding: 0; }");
  add("body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: var(--bg); color: var(--text); padding: 20px; line-height: 1.6; }");
  add(".container { max-width: 800px; margin: 0 auto; }");
  add(".app-header { padding-bottom: 20px; font-size: 2.5rem; color: var(--accent); font-weight: 700; text-align: center; }");
  add("#time-display { text-align: center; color: var(--text); font-size: 4.5rem; font-weight: 800; letter-spacing: -2px; }");
  add("#location-info { text-align: center; margin-top: 0px; margin-bottom: 0px; color: var(--text-muted); font-weight: 400; }");
  add(".greetings-text { text-align: center; color: var(--accent); margin-bottom: 24px; font-style: italic; font-weight: normal; letter-spacing: 1px; }");
  add(".identity-box { text-align: center; margin-top: 15px; padding: 12px; background: rgba(0,0,0,0.15); border-radius: 8px; border: 1px solid var(--border); }");
  add(".id-text { color: var(--accent); font-weight: 700; text-transform: uppercase; font-family: monospace; font-size: 1.1rem; margin-bottom: 2px; }");
  add(".ip-text { font-size: 0.9rem; color: var(--text-muted); font-family: monospace; margin-bottom: 5px; }");
  add(".status-badge { text-align: center; font-size: 0.85rem; font-weight: 600; text-transform: uppercase; letter-spacing: 1px; font-family: monospace; }");
  add("#pc-link-status { color: #10b981; }");
  add(".panel { background: var(--card); padding: 25px; border-radius: 12px; border: 1px solid var(--border); margin-bottom: 24px; }");
  add(".header-panel { background: rgba(59, 130, 246, 0.05); border: 1px solid var(--accent); }");
  add(".panel-title { margin-top: 0; color: var(--accent); font-size: 1.17em; }");
  add(".dashboard-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 15px; margin-top: 20px; }");
  add(".dashboard-grid > div { min-width: 0; }");
  add(".tile { background: rgba(15, 23, 42, 0.4); border: 1px dashed var(--border); padding: 20px; border-radius: 10px; text-align: center; }");
  add(".tile-icon { font-size: 2.2rem; margin-bottom: 8px; }");
  add(".tile-label { font-size: 0.75rem; color: var(--text-muted); text-transform: uppercase; letter-spacing: 1px; }");
  add(".tile-value { font-size: 1.6rem; font-weight: 600; color: var(--text); }");
  add(".date-val { font-size: 1.2rem; }");
  add(".no-data-tile { grid-column: 1 / -1; background: rgba(59, 130, 246, 0.05); border: 1px solid var(--accent); padding: 30px; color: var(--accent); border-radius: 10px; text-align: center; margin-top: 15px; }");
  add("label { display: block; margin-top: 15px; font-weight: 600; color: var(--text); }");
  add("input[type='text'], input[type='number'], input[type='time'], select { display: block; width: 100% !important; padding: 12px; margin: 8px 0; border: 1px solid var(--border); border-radius: 6px; background-color: #0f172a; color: var(--text); font-size: 15px; appearance: none; -webkit-appearance: none; }");
  add("select { background-image: url('data:image/svg+xml;utf8,<svg fill=\"%23f1f5f9\" height=\"24\" viewBox=\"0 0 24 24\" width=\"24\" xmlns=\"http://www.w3.org/2000/svg\"><path d=\"M7 10l5 5 5-5z\"/></svg>'); background-repeat: no-repeat; background-position: right 10px center; }");
  add("input[type='time'] { text-align: center; }");
  add("input[type='time']::-webkit-calendar-picker-indicator { filter: invert(1); cursor: pointer; }");
  add("input:focus, select:focus { border-color: var(--accent); outline: none; box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.2); }");
  add("input:disabled { opacity: 0.5; cursor: not-allowed; background-color: #1e293b; }");
  add("fieldset { border: 1px solid var(--border); border-radius: 8px; padding: 20px; margin-top: 15px; background: rgba(0,0,0,0.1); min-width: 0; overflow: hidden; }");
  add("legend { color: var(--accent); font-weight: 700; padding: 0 10px; font-size: 0.9rem; text-transform: uppercase; }");
  add("input[type='checkbox'], input[type='radio'] { accent-color: var(--accent); cursor: pointer; width: 16px; height: 16px; }");
  add(".checkbox-label { display: flex; align-items: center; gap: 8px; margin-top: 10px; cursor: pointer; font-weight: 600; }");
  add(".radio-group { display: flex; gap: 15px; margin-top: 8px; }");
  add(".radio-label { display: flex; align-items: center; gap: 6px; cursor: pointer; margin-top: 0; font-weight: normal; }");
  add(".anim-label { margin-top: 20px; margin-bottom: 10px; font-weight: 600; display: block; }");
  add(".anim-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-bottom: 15px; padding: 12px; background: rgba(255,255,255,0.05); border-radius: 8px; border: 1px solid rgba(255,255,255,0.1); }");
  add(".anim-item { display: flex; align-items: center; gap: 6px; cursor: pointer; font-size: 0.9em; margin-top: 0; }");
  add(".sortable-list { list-style: none; padding: 0; margin: 15px 0 0; border: 1px solid var(--border); border-radius: 8px; overflow: hidden; background: rgba(0,0,0,0.1); }");
  add(".sortable-item { padding: 12px 15px; border-bottom: 1px solid var(--border); display: flex; align-items: center; cursor: grab; color: var(--text); background: var(--card); transition: background 0.2s, opacity 0.2s; }");
  add(".sortable-item:last-child { border-bottom: none; }");
  add(".sortable-item:active { cursor: grabbing; }");
  add(".sortable-item.disabled { opacity: 0.4; background: transparent; cursor: default; }");
  add(".drag-handle { margin-right: 15px; color: var(--text-muted); font-size: 1.2rem; }");
  add(".sortable-item.dragging { opacity: 0.5; background: rgba(59, 130, 246, 0.2); }");
  add("button { background-color: var(--accent); color: white; padding: 16px; border: none; border-radius: 8px; cursor: pointer; margin-top: 8px; width: 100%; font-size: 1.1rem; font-weight: 700; transition: opacity 0.2s; }");
  add("button:hover { opacity: 0.9; }");
  add(".help-text { font-size: 0.8em; color: var(--text-muted); margin-top: 8px; }");
  add(".mt-0 { margin-top: 0 !important; }");
  add(".update-footer { text-align: center; font-size: 0.8rem; color: var(--text-muted); margin-top: 15px; font-family: monospace; }");
  add(".collapsible { transition: all 0.3s ease; }");
  add(".hidden { display: none !important; }");
  add("hr { border: 0; border-top: 1px solid var(--border); margin: 25px 0; }");
  add("@media (max-width: 600px) { .dashboard-grid { grid-template-columns: 1fr; } #time-display { font-size: 3.5rem; } }");
  add("</style></head><body><div class='container'>");

  add("<div class='app-header'>Tinytosh</div>");
  add("<div class='panel header-panel'><div id='time-display'>" + getCurrentTimeShort(config.time_format) + "</div>"); 
  add("<h2 id='location-info'>📍 --, -- (--)</h2>"); 
  add("<div id='greetings-text' class='greetings-text'></div>");
  
  String pairedPc = config.active_pc_id;
  int lastDash = pairedPc.lastIndexOf(':');
  if (lastDash > 3) pairedPc = pairedPc.substring(0, lastDash);

  bool isPcPaired = (pairedPc != "" && (millis() - pc.last_update < 5000));
  String pcStatus = isPcPaired ? ("🔒 Paired to " + pairedPc) : "";
  String ipAddress = WiFi.localIP().toString();

  add("<div class='identity-box'>");
  add("<div class='id-text'>" + config.device_id + "</div>");
  add("<div class='ip-text'>IP: " + ipAddress + "</div>");
  add("<div id='pc-link-status' class='status-badge'>" + pcStatus + "</div>");
  add("</div></div>");

  add("<form method='get' action='/save'>");

  add("<div class='panel'><h3 class='panel-title'>Global Settings</h3>");
  add("<label>Data Sync Interval (Mins):</label><input type='number' name='refresh_min' value='" + String(config.refresh_interval_min) + "'>");
  add("<label class='checkbox-label mt-0' style='margin-top: 10px !important;'><input type='checkbox' id='autoCycle' name='auto_cycle' value='1' " + String(config.screen_auto_cycle ? "checked" : "") + "> Cycle Screens Automatically</label>");
  add("<p class='help-text mt-0'>If disabled, screens will only change when you press the button.</p>");
  add("<label>Screen Cycle Interval (Secs):</label><input type='number' id='screenIntInput' name='screen_int' value='" + String(config.screen_interval_sec) + "'>");
  add("<label>Touch Sensor GPIO Pin:</label><input type='number' name='touch_pin' min='0' max='21' value='" + String(config.touch_pin) + "'>");
  add("<p class='help-text mt-0'>TTP223 output pin. Default is GPIO 10; avoid pins already used by OLED, USB, or boot wiring.</p>");

  add("<label class='anim-label'>Active Animations:</label>");
  add("<p class='help-text mt-0' style='margin-bottom: 10px;'>If 'None' is unchecked, select which animations to cycle through.</p>");
  add("<div class='anim-grid'>");
  
  const char* animLabels[] = {
    "🚫 None", "↔️ Slide Horizontal", "↕️ Slide Vertical", "👾 Dissolve (Noise)", "🎭 Curtain Open", "🎹 Venetian Blinds"
  };

  add("<input type='hidden' id='finalMask' name='anim_mask' value='" + String(config.anim_mask) + "'>");

  bool isNone = (config.anim_mask == 0);
  add("<label class='anim-item'>"); 
  add("<input type='checkbox' id='animNone' " + String(isNone ? "checked" : "") + ">" + String(animLabels[0]) + "</label>");

  for (int i = 1; i <= 5; i++) {
    bool isSet = (config.anim_mask & (1 << i));
    String checked = isSet ? "checked" : "";
    add("<label class='anim-item'><input type='checkbox' class='anim-chk' value='" + String(1 << i) + "' " + checked + ">" + String(animLabels[i]) + "</label>");
  }
  add("</div>");

  add("<label>Time Format:</label><div class='radio-group'>");
  add("<label class='radio-label'><input type='radio' name='time_format' value='24' " + String(config.time_format == "24" ? "checked" : "") + "> 24-Hour</label>");
  add("<label class='radio-label'><input type='radio' name='time_format' value='12' " + String(config.time_format == "12" ? "checked" : "") + "> 12-Hour</label></div>");
  add("<p class='help-text'>Format affects both the OLED display and the Web Panel.</p>");

  add("<hr>");

  add("<label class='checkbox-label mt-0'><input type='checkbox' id='autoDetect' name='auto_detect' value='1' " + String(config.auto_detect ? "checked" : "") + "> Detect Location Automatically (IP)</label>");
  add("<p class='help-text mt-0'>Uses your IP address to determine city, coordinates, and timezone.</p>");

  add("<fieldset id='manualFields' class='collapsible'>");
  add("<legend>Manual Location Entry</legend>");
  add("<label>City Name:</label><input type='text' name='city' value='" + config.city + "'>");
  
  add("<div class='dashboard-grid mt-0'>");
  add("  <div><label class='mt-0'>Latitude:</label><input type='number' step='any' name='latitude' value='" + String(config.latitude, 4) + "'></div>");
  add("  <div><label class='mt-0'>Longitude:</label><input type='number' step='any' name='longitude' value='" + String(config.longitude, 4) + "'></div>");
  add("</div>");

  add("<div class='dashboard-grid mt-0'>");
  add("  <div><label class='mt-0'>Country:</label><select name='country_code'>");
  for(auto c : allCountries) {
      add("<option value='" + String(c.code) + "'" + (String(config.country_code) == String(c.code) ? " selected" : "") + ">" + String(c.name) + "</option>");
  }
  add("  </select></div>");

  add("  <div><label class='mt-0'>Timezone:</label><select name='timezone'>");
  DynamicJsonDocument tzDoc(6144); 
  deserializeJson(tzDoc, POSIX_TIMEZONE_MAP); 
  for (JsonPair p : tzDoc.as<JsonObject>()) {
      String key = p.key().c_str();
      add("<option value='" + key + "'" + (key == config.timezone ? " selected" : "") + ">" + key + "</option>");
  }
  add("  </select></div>");
  add("</div></fieldset><hr>");
  
  add("<label class='checkbox-label mt-0'><input type='checkbox' id='nightMode' name='night_mode' value='1' " + String(config.night_mode ? "checked" : "") + "> Enable Night Mode</label>");
  add("<p class='help-text mt-0'>Set a quiet schedule to pause animations, dim the screen, or save API calls.</p>");

  add("<fieldset id='nightFields' class='collapsible'>");
  add("<legend>Night Schedule</legend>");
  add("<label class='mt-0'>Screen Action:</label><select name='night_action' id='nightActionSelect' style='margin-top: 8px;'>");
  add("<option value='0' " + String(config.night_action == 0 ? "selected" : "") + ">No Visual Change</option>");
  add("<option value='1' " + String(config.night_action == 1 ? "selected" : "") + ">Dim Display</option>");
  add("<option value='2' " + String(config.night_action == 2 ? "selected" : "") + ">Turn Display Off</option>");
  add("<option value='3' " + String(config.night_action == 3 ? "selected" : "") + ">Dim then Turn Off</option>");
  add("</select>");

  add("<div id='dimStartContainer' style='display: none;'>");
  add("  <label>Dim Start Time:</label><input type='time' name='night_dim_start' value='" + String(config.night_dim_start) + "'>");
  add("</div>");

  add("<div class='dashboard-grid'>"); 
  add("  <div><label class='mt-0'>Start Time:</label><input type='time' name='night_start' value='" + String(config.night_start) + "'></div>");
  add("  <div><label class='mt-0'>End Time:</label><input type='time' name='night_end' value='" + String(config.night_end) + "'></div>");
  add("</div></fieldset></div>");

  add("<div class='panel'><h3 class='panel-title'>Screen Display Order</h3>");
  add("<p class='help-text mt-0' style='margin-bottom: 15px;'>Drag and drop to rearrange. Disabled screens are locked at the bottom.</p>");
  add("<ul id='sortable-list' class='sortable-list'>");
  
  for (int screenId = 0; screenId < NUM_SCREENS; screenId++) {
    String targetId = "";
    switch(screenId) {
      case SCREEN_TIME: targetId = "showTime"; break;
      case SCREEN_CALENDAR: targetId = "showCalendar"; break;
      case SCREEN_WEATHER: targetId = "showWeather"; break;
      case SCREEN_AIR_QUALITY: targetId = "showAQI"; break;
      case SCREEN_CRYPTO: targetId = "showCrypto"; break;
      case SCREEN_CURRENCY: targetId = "showCurrency"; break;
      case SCREEN_STOCK: targetId = "showStock"; break;
      case SCREEN_PC_MONITOR: targetId = "showPc"; break;
      case SCREEN_PC_MEDIA: targetId = "showMedia"; break;
      case SCREEN_BAMBU: targetId = "showBambu"; break;
    }
    
    add("<li class='sortable-item' data-id='" + String(screenId) + "' data-target='" + targetId + "' draggable='true'>");
    add("<span class='drag-handle'>☰</span>" + String(SCREEN_NAMES[screenId]) + "</li>");
  }
  add("</ul><input type='hidden' name='screen_order' id='screenOrderInput' value=''></div>");

  add("<div id='dynamic-panels-container'>");
  for (int i = 0; i < NUM_SCREENS; i++) {
      int screenId = config.screen_order[i];

      switch (screenId) {
          case SCREEN_TIME: {
              add("<div class='panel' id='panel-" + String(screenId) + "'>");
              add("<label class='checkbox-label mt-0'><input type='checkbox' id='showTime' name='show_time' value='1' " + String(config.show_time ? "checked" : "") + "> Time Screen</label>");
              add("<div id='timeContent' class='collapsible'>");
              
              add("<div class='dashboard-grid'>");
              add("<div class='tile'><div class='tile-icon'>🕒</div><div class='tile-value' id='preview-time'>" + getCurrentTimeShort(config.time_format) + "</div><div class='tile-label'>Current Time</div></div>");
              add("<div class='tile'><div class='tile-icon'>🌐</div><div class='tile-value' id='preview-tz' style='font-size:1.2rem'>" + config.timezone + "</div><div class='tile-label'>Timezone</div></div>");
              add("</div>");
              
              add("<label class='checkbox-label'><input type='checkbox' name='date_display' value='1' " + String(config.date_display ? "checked" : "") + "> Display Date Below Time</label>");
              add("</div></div>");
              break;
          }

          case SCREEN_CALENDAR: {
              add("<div class='panel' id='panel-" + String(screenId) + "'>");
              add("<label class='checkbox-label mt-0'><input type='checkbox' id='showCalendar' name='show_calendar' value='1' " + String(config.show_calendar ? "checked" : "") + "> Calendar Screen</label>");
              add("<div id='calendarContent' class='collapsible'>");
              
              String holText = (state->calendar.count > 0) ? String(state->calendar.count) : "No holiday data";
              
              add("<div class='dashboard-grid'>");
              add("<div class='tile'><div class='tile-icon'>📅</div><div class='tile-value date-val' id='preview-date'>" + getFullDate() + "</div><div class='tile-label'>Current Date</div></div>");
              add("<div class='tile'><div class='tile-icon'>🎉</div><div class='tile-value' id='preview-hol' style='font-size:1.2rem'>" + holText + "</div><div class='tile-label'>Holidays Loaded</div></div>");
              add("</div>");
              
              add("<label>Start Week On:</label><div class='radio-group'>");
              add("<label class='radio-label'><input type='radio' name='cal_start' value='mon' " + String(config.calendar_start_day == "mon" ? "checked" : "") + "> Monday</label>");
              add("<label class='radio-label'><input type='radio' name='cal_start' value='sun' " + String(config.calendar_start_day == "sun" ? "checked" : "") + "> Sunday</label></div>");
              
              add("<label class='checkbox-label'><input type='checkbox' name='cal_hol' value='1' " + String(config.calendar_show_holidays ? "checked" : "") + "> Show National Holidays</label>");
              add("<label class='checkbox-label'><input type='checkbox' name='cal_min' value='1' " + String(config.calendar_minimal ? "checked" : "") + "> Minimalistic Mode (Hide grid)</label>");
              add("</div></div>");
              break;
          }
          
          case SCREEN_WEATHER: {
              add("<div class='panel' id='panel-" + String(screenId) + "'>");
              add("<label class='checkbox-label mt-0'><input type='checkbox' id='showWeather' name='show_weather' value='1' " + String(config.show_weather ? "checked" : "") + "> Weather Screen</label>");
              add("<div id='weatherContent' class='collapsible'>");
              
              if (!weatherValid) {
                  add("<div id='weather-no-data' class='no-data-tile'>☁️ Weather data will be available after sync</div><div id='weather-grid' class='hidden'>");
              } else {
                  add("<div id='weather-no-data' class='no-data-tile hidden'>☁️ Weather data will be available after sync</div><div id='weather-grid'>");
              }
              
              add("<div class='dashboard-grid'>");
              add("<div class='tile'><div class='tile-icon' id='icon-temp'>" + getWeatherIcon(weather.weather_code) + "</div><div class='tile-value' id='value-temp'>" + String(weather.temp, 1) + " °" + config.temp_unit + "</div><div class='tile-label'>Temperature</div></div>");
              add("<div class='tile'><div class='tile-icon'>🤒</div><div class='tile-value' id='value-feels'>" + String(weather.apparent_temperature, 1) + " °" + config.temp_unit + "</div><div class='tile-label'>Feels Like</div></div>");
              add("<div class='tile'><div class='tile-icon'>💧</div><div class='tile-value' id='value-hum'>" + String(weather.humidity) + "%</div><div class='tile-label'>Humidity</div></div>");
              add("<div class='tile'><div class='tile-icon'>💨</div><div class='tile-value' id='value-wind'>" + String(weather.wind_speed, 1) + " km/h</div><div class='tile-label'>Wind Speed</div></div>");
              add("</div><div class='update-footer' id='weather-upd'>Last Update: " + weather.update_time + "</div></div>");
              
              add("<label>Temperature Unit:</label><div class='radio-group'>");
              add("<label class='radio-label'><input type='radio' name='temp_unit' value='C' " + String(config.temp_unit == "C" ? "checked" : "") + "> °C</label>");
              add("<label class='radio-label'><input type='radio' name='temp_unit' value='F' " + String(config.temp_unit == "F" ? "checked" : "") + "> °F</label></div>");
              
              add("<label class='checkbox-label'><input type='checkbox' name='round_temps' value='1' " + String(config.round_temps ? "checked" : "") + "> Round Temperature Values</label>");
              add("</div></div>");
              break;
          }

          case SCREEN_AIR_QUALITY: {
              add("<div class='panel' id='panel-" + String(screenId) + "'>");
              add("<label class='checkbox-label mt-0'><input type='checkbox' id='showAQI' name='show_aqi' value='1' " + String(config.show_aqi ? "checked" : "") + "> Air Quality Screen</label>");
              add("<div id='aqiContent' class='collapsible'>");

              if (!aqiValid) {
                  add("<div id='aqi-no-data' class='no-data-tile'>🍃 Air quality data will be available after sync</div><div id='aqi-grid' class='hidden'>");
              } else {
                  add("<div id='aqi-no-data' class='no-data-tile hidden'>🍃 Air quality data will be available after sync</div><div id='aqi-grid'>");
              }
              
              add("<div class='dashboard-grid'>");
              add("<div class='tile'><div class='tile-icon'>🍃</div><div class='tile-value' id='value-aqi'>" + String(aqi.aqi) + "</div><div class='tile-label'>" + aqi.status + " Index</div></div>");
              add("<div class='tile'><div class='tile-icon'>🌫️</div><div class='tile-value' id='value-pm25'>" + String(aqi.pm25, 1) + " <small>µg</small></div><div class='tile-label'>PM 2.5</div></div>");
              add("<div class='tile'><div class='tile-icon'>🏭</div><div class='tile-value' id='value-pm10'>" + String(aqi.pm10, 1) + " <small>µg</small></div><div class='tile-label'>PM 10</div></div>");
              add("<div class='tile'><div class='tile-icon'>🧪</div><div class='tile-value' id='value-no2'>" + String(aqi.no2, 1) + " <small>µg</small></div><div class='tile-label'>Nitrogen Dioxide</div></div>");
              add("</div><div class='update-footer' id='aqi-upd'>Last Update: " + weather.update_time + "</div></div>");

              add("<label>AQI Standard:</label><div class='radio-group'>");
              add("<label class='radio-label'><input type='radio' name='aqi_type' value='US' " + String(config.aqi_type == "US" ? "checked" : "") + "> US Standard</label>");
              add("<label class='radio-label'><input type='radio' name='aqi_type' value='EU' " + String(config.aqi_type == "EU" ? "checked" : "") + "> European Standard</label></div>");
              add("<p class='help-text mt-0'>EU: 0-100+ scale | US: 0-500 scale</p>");

              add("</div></div>");
              break;
          }

          case SCREEN_STOCK: {
              add("<div class='panel' id='panel-" + String(screenId) + "'>");
              add("<label class='checkbox-label mt-0'><input type='checkbox' id='showStock' name='show_stock' value='1' " + String(config.show_stock ? "checked" : "") + "> Stock Tracking Screen</label>");
              add("<div id='stockContent' class='collapsible'>");
              
              if (!stockValid) {
                  add("<div id='stock-no-data' class='no-data-tile'>📈 Stock data will be available after sync</div><div id='stock-grid' class='hidden'>");
              } else {
                  add("<div id='stock-no-data' class='no-data-tile hidden'>📈 Stock data will be available after sync</div><div id='stock-grid'>");
              }
              
              add("<div class='dashboard-grid'>");
              add("<div class='tile'><div class='tile-icon'>📊</div><div class='tile-value' id='stock-price'>$" + String(stock.price, 2) + "</div><div class='tile-label' id='stock-sym'>" + stock.symbol + " Price</div></div>");
              add("<div class='tile'><div class='tile-icon' id='stock-trend-icon'>" + String(stock.percent_change >= 0 ? "📈" : "📉") + "</div><div class='tile-value' id='stock-change'>" + String(stock.percent_change, 2) + "%</div><div class='tile-label'>Daily Change</div></div>");
              add("</div><div class='update-footer' id='stock-upd'>Last Update: " + weather.update_time + "</div></div>");

              add("<label>Track Stock/ETF:</label><select name='stock_symbol'>");
              for(auto s : topStocks) {
                  add("<option value='" + String(s.ticker) + "' " + (String(config.stock_symbol) == String(s.ticker) ? "selected" : "") + ">" + String(s.name) + " - " + String(s.ticker) + "</option>");
              }
              add("</select>");
              add("<label class='checkbox-label'><input type='checkbox' name='stock_fn' value='1' " + String(config.stock_fn ? "checked" : "") + "> Display Full Company Name</label>");
              add("</div></div>");
              break;
          }

          case SCREEN_CRYPTO: {
              add("<div class='panel' id='panel-" + String(screenId) + "'>");
              add("<label class='checkbox-label mt-0'><input type='checkbox' id='showCrypto' name='show_crypto' value='1' " + String(config.show_crypto ? "checked" : "") + "> Crypto Tracking Screen</label>");
              add("<div id='cryptoContent' class='collapsible'>");
              
              if (!cryptoValid) {
                  add("<div id='crypto-no-data' class='no-data-tile'>💰 Crypto data will be available after sync</div><div id='crypto-grid' class='hidden'>");
              } else {
                  add("<div id='crypto-no-data' class='no-data-tile hidden'>💰 Crypto data will be available after sync</div><div id='crypto-grid'>");
              }
              
              add("<div class='dashboard-grid'>");
              add("<div class='tile'><div class='tile-icon'>₿</div><div class='tile-value' id='crypto-price'>" + String((int)round(crypto.price_usd)) + "$</div><div class='tile-label' id='crypto-sym'>" + crypto.symbol + " Price</div></div>");
              add("<div class='tile'><div class='tile-icon' id='crypto-trend-icon'>" + String(crypto.percent_change_24h >= 0 ? "📈" : "📉") + "</div><div class='tile-value' id='crypto-change'>" + String(crypto.percent_change_24h, 1) + "%</div><div class='tile-label'>24h Change</div></div>");
              add("</div><div class='update-footer' id='crypto-upd'>Last Update: " + weather.update_time + "</div></div>");
              
              add("<label>Track Cryptocurrency:</label><select name='crypto_id'>");
              for(auto coin : topCoins) {
                  add("<option value='" + String(coin.id) + "' " + (config.crypto_id == coin.id ? "selected" : "") + ">" + String(coin.sym) + "</option>");
              }
              add("</select>");
              add("<label class='checkbox-label'><input type='checkbox' name='crypto_fn' value='1' " + String(config.crypto_fn ? "checked" : "") + "> Display Full Coin Name</label>");
              add("</div></div>");
              break;
          }

          case SCREEN_CURRENCY: {
              add("<div class='panel' id='panel-" + String(screenId) + "'>");
              add("<label class='checkbox-label mt-0'><input type='checkbox' id='showCurrency' name='show_currency' value='1' " + String(config.show_currency ? "checked" : "") + "> Currency Exchange Screen</label>");
              add("<div id='currencyContent' class='collapsible'>");
              
              if (!currencyValid) {
                  add("<div id='currency-no-data' class='no-data-tile'>💱 Currency data will be available after sync</div><div id='currency-grid' class='hidden'>");
              } else {
                  add("<div id='currency-no-data' class='no-data-tile hidden'>💱 Currency data will be available after sync</div><div id='currency-grid'>");
              }
              
              add("<div class='dashboard-grid'>");
              float displayRate = currency.rate * config.currency_multiplier;
              int decimals = 0;
              if (displayRate < 10.0) decimals = 3;
              else if (displayRate < 100.0) decimals = 2;
              else if (displayRate < 1000.0) decimals = 1;

              add("<div class='tile'><div class='tile-icon'>💵</div><div class='tile-value' id='currency-base-val'>" + String(config.currency_multiplier) + " " + currency.base + "</div><div class='tile-label'>Base Amount</div></div>");
              add("<div class='tile'><div class='tile-icon'>💱</div><div class='tile-value' id='currency-target-val'>" + String(displayRate, decimals) + " " + currency.target + "</div><div class='tile-label'>Exchange Rate</div></div>");
              add("</div><div class='update-footer' id='currency-upd'>Last Update: " + weather.update_time + "</div></div>");

              add("<div class='dashboard-grid mt-0'>");
              
              add("<div><label class='mt-0'>Base Currency:</label><select name='currency_base'>");
              for(auto c : allCurrencies) {
                  String codeDisplay = String(c.code).substring(0, 3);
                  codeDisplay.toUpperCase(); 
                  add("<option value='" + String(c.code) + "' " + (String(config.currency_base) == String(c.code) ? "selected" : "") + ">" + codeDisplay + "</option>");
              }
              add("</select></div>");

              add("<div><label class='mt-0'>Target Currency:</label><select name='currency_target'>");
              for(auto c : allCurrencies) {
                  String codeDisplay = String(c.code).substring(0, 3);
                  codeDisplay.toUpperCase(); 
                  add("<option value='" + String(c.code) + "' " + (String(config.currency_target) == String(c.code) ? "selected" : "") + ">" + codeDisplay + "</option>");
              }
              add("</select></div></div>");

              add("<label>Multiplier Amount:</label><select name='currency_multiplier'>");
              int multipliers[] = {1, 10, 100, 1000, 10000, 100000};
              for (int m : multipliers) {
                  add("<option value='" + String(m) + "' " + (config.currency_multiplier == m ? "selected" : "") + ">" + String(m) + "</option>");
              }
              add("</select>");
              add("<label class='checkbox-label'><input type='checkbox' name='currency_fn' value='1' " + String(config.currency_fn ? "checked" : "") + "> Display Full Currency Name</label>");
              add("</div></div>");
              break;
          }

          case SCREEN_PC_MONITOR: {
              add("<div class='panel' id='panel-" + String(screenId) + "'>");
              add("<label class='checkbox-label mt-0'><input type='checkbox' id='showPc' name='show_pc' value='1' " + String(config.show_pc ? "checked" : "") + "> PC Monitoring Screen</label>");
              add("<div id='pcContent' class='collapsible'>");
              
              if (!pcValid) {
                  add("<div id='pc-no-data' class='no-data-tile'>🖥️ PC data will be available after sync</div><div id='pc-grid' class='hidden'>");
              } else {
                  add("<div id='pc-no-data' class='no-data-tile hidden'>🖥️ PC data will be available after sync</div><div id='pc-grid'>");
              }
              
              add("<div class='dashboard-grid'>");
              add("<div class='tile'><div class='tile-icon'>📊</div><div class='tile-value' id='pc-cpu'>" + String((int)round(pc.cpu_percent)) + "%</div><div class='tile-label'>CPU Usage</div></div>");
              add("<div class='tile'><div class='tile-icon'>🧠</div><div class='tile-value' id='pc-ram'>" + String((int)round(pc.mem_percent)) + "%</div><div class='tile-label'>RAM Usage</div></div>");
              add("<div class='tile'><div class='tile-icon'>💽</div><div class='tile-value' id='pc-disk'>" + String((int)round(pc.disk_percent)) + "%</div><div class='tile-label'>Disk Usage</div></div>");
              add("<div class='tile'><div class='tile-icon'>⬇️</div><div class='tile-value' id='pc-net'>" + String((int)round(pc.net_down_kb)) + " KB/s</div><div class='tile-label'>Download</div></div>");      
              add("</div></div>");
              add("<label class='checkbox-label'><input type='checkbox' name='hide_empty_pc' value='1' " + String(config.hide_empty_pc ? "checked" : "") + "> Hide empty screen</label>");
              add("<p class='help-text mt-0'>Screen is excluded from rotation when there is no data.</p>");
              add("</div></div>");
              break;
          }

          case SCREEN_PC_MEDIA: {
              add("<div class='panel' id='panel-" + String(screenId) + "'>");
              add("<label class='checkbox-label mt-0'><input type='checkbox' id='showMedia' name='show_media' value='1' " + String(config.show_media ? "checked" : "") + "> PC Media Screen</label>");
              add("<div id='mediaContent' class='collapsible'>");

              bool isMediaValid = (media.name.length() > 0 && media.author.length() > 0);
              if (!isMediaValid) {
                  add("<div id='media-no-data' class='no-data-tile'>🎵 Media data will be available after sync and/or when media is played</div><div id='media-grid' class='hidden'>");
              } else {
                  add("<div id='media-no-data' class='no-data-tile hidden'>🎵 Media data will be available after sync and/or when media is played</div><div id='media-grid'>");
              }

              add("<div class='dashboard-grid'>");
              add("<div class='tile'><div class='tile-icon'>🎵</div><div class='tile-value' id='web-media-status' style='font-size:1.2rem'>" + media.status + "</div><div class='tile-label'>Status</div></div>");
              add("<div class='tile'><div class='tile-icon'>🎧</div><div class='tile-value' id='web-media-name' style='font-size:1.2rem'>" + media.name + "</div><div class='tile-label'>Track</div></div>");
              add("<div class='tile'><div class='tile-icon'>👤</div><div class='tile-value' id='web-media-author' style='font-size:1.2rem'>" + media.author + "</div><div class='tile-label'>Author</div></div>");
              add("<div class='tile'><div class='tile-icon'>💿</div><div class='tile-value' id='web-media-album' style='font-size:1.2rem'>" + media.album + "</div><div class='tile-label'>Album</div></div>");
              add("</div></div>");
              add("<label class='checkbox-label'><input type='checkbox' name='hide_empty_media' value='1' " + String(config.hide_empty_media ? "checked" : "") + "> Hide empty screen</label>");
              add("<p class='help-text mt-0'>Screen is excluded from rotation when there is no data.</p>");
              add("</div></div>");
              break;
          }

          case SCREEN_BAMBU: {
              add("<div class='panel' id='panel-" + String(screenId) + "'>");
              add("<label class='checkbox-label mt-0'><input type='checkbox' id='showBambu' name='show_bambu' value='1' " + String(config.show_bambu ? "checked" : "") + "> Bambu 3D Printer Screen</label>");
              add("<div id='bambuContent' class='collapsible'>");
              
              bool isBambuKnown = (state->bambu.status != "SYNCING");
              if (!isBambuKnown) {
                  add("<div id='bambu-no-data' class='no-data-tile'>🖨️ Printer data will be available after connection is established</div><div id='bambu-grid' class='hidden'>");
              } else {
                  add("<div id='bambu-no-data' class='no-data-tile hidden'>🖨️ Printer data will be available after connection is established</div><div id='bambu-grid'>");
              }
              
              add("<div class='dashboard-grid'>");
              add("<div class='tile'><div class='tile-icon'>🖨️</div><div class='tile-value' id='bambu-status' style='font-size:1.2rem'>" + state->bambu.status + "</div><div class='tile-label'>Status</div></div>");
              add("<div class='tile'><div class='tile-icon'>⏳</div><div class='tile-value' id='bambu-prog' style='font-size:1.1rem'>" + String(state->bambu.progress) + "% | " + String(state->bambu.time_left) + "m<br><span style='font-size:0.9rem'>Layers: " + String(state->bambu.layer) + "/" + String(state->bambu.total_layers) + "</span></div><div class='tile-label'>Progress</div></div>");
              add("<div class='tile'><div class='tile-icon'>🌡️</div><div class='tile-value' id='bambu-temps' style='font-size:1.1rem'>Nozzle: " + String(state->bambu.nozzle_temp, 1) + "/" + String(state->bambu.nozzle_target, 1) + "<br>Bed: " + String(state->bambu.bed_temp, 1) + "/" + String(state->bambu.bed_target, 1) + "</div><div class='tile-label'>Temperatures</div></div>");
              add("<div class='tile'><div class='tile-icon'>💨</div><div class='tile-value' id='bambu-fans' style='font-size:1.2rem'>Part: " + String(state->bambu.fan_part) + " | Aux: " + String(state->bambu.fan_aux) + "</div><div class='tile-label'>Fans</div></div>");
              add("</div></div>");

              add("<label class='checkbox-label'><input type='checkbox' name='hide_empty_bambu' value='1' " + String(config.hide_empty_bambu ? "checked" : "") + "> Hide empty screen</label>");
              add("<p class='help-text mt-0'>Screen is excluded from rotation when printer is offline.</p>");

              add("<label>Printer IP Address:</label><input type='text' name='bambu_ip' placeholder='e.g. 192.168.0.100' value='" + config.bambu_ip + "'>");
              add("<label>Printer Serial Number:</label><input type='text' name='bambu_sn' placeholder='e.g. 00M...' value='" + config.bambu_sn + "'>");
              add("<label>Printer Access Code:</label><input type='text' name='bambu_code' placeholder='e.g. 1234abcd' value='" + config.bambu_code + "'>");
              add("</div></div>");
              break;
          }
      }
  }

  add("</div>");
  add("<button type='submit'>💾 Save & Apply All Settings</button></form>");
  
  add("<script>");
  add("let formDirty = false;");
  add("function updateVisibility(){");
  add("  var pairs = [['autoDetect','manualFields',true], ['nightMode','nightFields',false], ['showTime', 'timeContent',false], ['showCalendar', 'calendarContent',false], ['showWeather','weatherContent',false], ['showPc','pcContent',false], ['showCrypto','cryptoContent',false], ['showCurrency','currencyContent',false], ['showStock','stockContent',false], ['showAQI','aqiContent',false], ['showMedia','mediaContent',false], ['showBambu','bambuContent',false]];");  
  add("  pairs.forEach(p => {");
  add("    var ch = document.getElementById(p[0]); if(!ch) return;");
  add("    var target = document.getElementById(p[1]);");
  add("    var shouldHide = p[2] ? ch.checked : !ch.checked;");
  add("    target.className = shouldHide ? 'collapsible hidden' : 'collapsible';");
  add("    target.querySelectorAll('input, select').forEach(el => el.disabled = shouldHide);");
  add("  });");

  add("  var ac = document.getElementById('autoCycle');");
  add("  var si = document.getElementById('screenIntInput');");
  add("  if(ac && si) si.disabled = !ac.checked;");
  add("}");

  add("function updateNightAction() {");
  add("  var action = document.getElementById('nightActionSelect').value;");
  add("  var dimCont = document.getElementById('dimStartContainer');");
  add("  if (action === '3') { dimCont.style.display = 'block'; } else { dimCont.style.display = 'none'; }");
  add("}");
  add("document.getElementById('nightActionSelect').addEventListener('change', updateNightAction);");
  add("updateNightAction();");
  
  add("['autoDetect', 'nightMode', 'showTime', 'showCalendar', 'showWeather', 'showPc', 'showCrypto', 'showCurrency', 'showStock', 'showAQI', 'showMedia', 'showBambu', 'autoCycle'].forEach(id => { var el=document.getElementById(id); if(el) el.addEventListener('change', updateVisibility); });");
  add("updateVisibility();");

  add("const countryGreetings = {");
  add("  'BY': 'Жыве Беларусь ⚪🔴⚪', 'UA': 'Слава Україні 🇺🇦', 'RU': 'Россия Будет Свободной ⚪🔵⚪',");
  add("  'GB': 'Cheers, Britain 🇬🇧', 'US': 'Howdy, America 🇺🇸', 'PL': 'Dzień dobry, Polsko 🇵🇱',");
  add("  'CA': 'Hello, Canada 🇨🇦', 'AU': 'G\\'day, Australia 🇦🇺', 'FR': 'Bonjour, France 🇫🇷',");
  add("  'DE': 'Hallo, Deutschland 🇩🇪', 'IT': 'Viva l\\'Italia 🇮🇹', 'ES': 'Viva España 🇪🇸',");
  add("  'JP': 'Konnichiwa, Japan 🇯🇵', 'BR': 'Olá, Brasil 🇧🇷', 'IN': 'Namaste, India 🇮🇳',");
  add("  'MX': 'Viva México 🇲🇽', 'ZA': 'Sawubona, South Africa 🇿🇦', 'NZ': 'Kia Ora, New Zealand 🇳🇿',");
  add("  'IE': 'Dia dhuit, Ireland 🇮🇪', 'CH': 'Grüezi, Switzerland 🇨🇭', 'NL': 'Hallo, Nederland 🇳🇱',");
  add("  'KR': 'Annyeonghaseyo, Korea 🇰🇷', 'GR': 'Yassou, Greece 🇬🇷'");
  add("};");

  add("function updateLiveHeader() {");
  add("  const cInput = document.querySelector('input[name=\"city\"]');");
  add("  const cSel = document.querySelector('select[name=\"country_code\"]');");
  add("  const tSel = document.querySelector('select[name=\"timezone\"]');");
  add("  const city = (cInput && cInput.value) ? cInput.value : '--';");
  add("  const cName = (cSel && cSel.selectedIndex >= 0) ? cSel.options[cSel.selectedIndex].text : '--';");
  add("  const cCode = cSel ? cSel.value : null;");
  add("  const tz = (tSel && tSel.value) ? tSel.value : '--';");
  add("  const locInfo = document.getElementById('location-info');");
  add("  if (locInfo && city !== '--') locInfo.innerText = '📍 ' + city + ', ' + cName + ' (' + tz + ')';");
  add("  const greetingElement = document.getElementById('greetings-text');");
  add("  if (greetingElement) {");
  add("    if (cCode && countryGreetings[cCode]) { greetingElement.innerText = countryGreetings[cCode]; greetingElement.style.display = 'block'; }");
  add("    else { greetingElement.style.display = 'none'; }");
  add("  }");
  add("}");
  
  add("document.querySelector('input[name=\"city\"]').addEventListener('input', updateLiveHeader);");
  add("document.querySelector('select[name=\"country_code\"]').addEventListener('change', updateLiveHeader);");
  add("document.querySelector('select[name=\"timezone\"]').addEventListener('change', updateLiveHeader);");

  add("function toggleNone() {");
  add("  const noneBox = document.getElementById('animNone');");
  add("  const others = document.querySelectorAll('.anim-chk');");
  add("  others.forEach(cb => {");
  add("    cb.disabled = noneBox.checked;");
  add("    if(noneBox.checked) cb.checked = false;");
  add("    cb.parentElement.style.opacity = noneBox.checked ? '0.5' : '1';");
  add("  });");
  add("}");

  add("function checkSafetyNet() {");
  add("  if(!document.getElementById('animNone').checked) {");
  add("    let count = 0;");
  add("    document.querySelectorAll('.anim-chk').forEach(cb => { if(cb.checked) count++; });");
  add("    if(count === 0) {");
  add("      document.getElementById('animNone').checked = true;");
  add("      toggleNone();");
  add("    }");
  add("  }");
  add("}");

  add("const nb = document.getElementById('animNone');");
  add("if(nb) nb.addEventListener('change', toggleNone);");
  add("document.querySelectorAll('.anim-chk').forEach(cb => { cb.addEventListener('change', checkSafetyNet); });");
  add("toggleNone();");

  add("const list = document.getElementById('sortable-list');");
  add("const orderInput = document.getElementById('screenOrderInput');");

  add("function syncScreenOrder() {");
  add("  const items = [...list.querySelectorAll('.sortable-item')];");
  add("  let enabled = [], disabled = [];");
  add("  items.forEach(item => {");
  add("    const targetId = item.getAttribute('data-target');");
  add("    const cb = document.getElementById(targetId);");
  add("    if (cb && cb.checked) {");
  add("      item.classList.remove('disabled'); item.setAttribute('draggable', 'true'); enabled.push(item);");
  add("    } else {");
  add("      item.classList.add('disabled'); item.removeAttribute('draggable'); disabled.push(item);");
  add("    }");
  add("  });");
  add("  list.innerHTML = '';");
  add("  enabled.forEach(el => list.appendChild(el));"); 
  add("  disabled.forEach(el => list.appendChild(el));"); 
  add("  updateOrderValue();");
  add("}");

  add("function reorderPhysicalPanels(orderCsv) {");
  add("  const container = document.getElementById('dynamic-panels-container');");
  add("  if (!container || !orderCsv) return;");
  add("  const orderArr = orderCsv.split(',');");
  add("  orderArr.forEach(id => {");
  add("    const panel = document.getElementById('panel-' + id);");
  add("    if (panel) container.appendChild(panel);");
  add("  });");
  add("}");

  add("function updateOrderValue() {");
  add("  const items = [...list.querySelectorAll('.sortable-item')];");
  add("  orderInput.value = items.map(item => item.getAttribute('data-id')).join(',');");
  add("  reorderPhysicalPanels(orderInput.value);");
  add("}");

  add("const panelCheckboxes = ['showTime', 'showCalendar', 'showWeather', 'showAQI', 'showCrypto', 'showCurrency', 'showStock', 'showPc', 'showMedia', 'showBambu'];");
  add("panelCheckboxes.forEach(id => { const el = document.getElementById(id); if (el) el.addEventListener('change', syncScreenOrder); });");

  add("function getDragAfterEl(y) {");
  add("  return [...list.querySelectorAll('.sortable-item:not(.dragging):not(.disabled)')].reduce((closest, child) => {");
  add("    const box = child.getBoundingClientRect();");
  add("    const offset = y - box.top - box.height / 2;");
  add("    if (offset < 0 && offset > closest.offset) return { offset: offset, element: child };");
  add("    else return closest;");
  add("  }, { offset: Number.NEGATIVE_INFINITY }).element;");
  add("}");

  add("function moveItem(y) {");
  add("  const draggable = document.querySelector('.dragging');");
  add("  if (!draggable) return;");
  add("  const afterEl = getDragAfterEl(y);");
  add("  if (afterEl == null) {");
  add("    const firstDis = list.querySelector('.disabled');");
  add("    if (firstDis) list.insertBefore(draggable, firstDis);");
  add("    else list.appendChild(draggable);");
  add("  } else { list.insertBefore(draggable, afterEl); }");
  add("}");

  add("list.addEventListener('dragstart', e => { if (e.target.classList.contains('disabled')) { e.preventDefault(); return; } e.target.classList.add('dragging'); });");
  add("list.addEventListener('dragend', e => { e.target.classList.remove('dragging'); updateOrderValue(); formDirty = true; });");
  add("list.addEventListener('dragover', e => { e.preventDefault(); moveItem(e.clientY); });");

  add("list.addEventListener('touchstart', e => {");
  add("  const item = e.target.closest('.sortable-item');");
  add("  if (!item || item.classList.contains('disabled')) return;");
  add("  item.classList.add('dragging');");
  add("}, {passive: false});");

  add("list.addEventListener('touchmove', e => {");
  add("  if (!document.querySelector('.dragging')) return;");
  add("  e.preventDefault(); moveItem(e.touches[0].clientY);");
  add("}, {passive: false});");

  add("list.addEventListener('touchend', e => {");
  add("  const dragging = document.querySelector('.dragging');");
  add("  if (dragging) { dragging.classList.remove('dragging'); updateOrderValue(); formDirty = true; }");
  add("});");

  add("syncScreenOrder();");
  
  add("document.querySelector('form').addEventListener('submit', function(e) {");
  add("  let mask = 0;");
  add("  document.querySelectorAll('.anim-chk').forEach(cb => { if(cb.checked) mask += parseInt(cb.value); });");
  add("  document.getElementById('finalMask').value = mask;");
  add("});");
  
  add("formDirty = false;");
  add("document.querySelector('form').addEventListener('input', () => formDirty = true);");
  add("document.querySelector('form').addEventListener('change', () => formDirty = true);");

  add("function updateData() { fetch('/update').then(r => r.json()).then(d => {");
  add("  const set = (id, val, html=false) => { const el = document.getElementById(id); if(el) { if(html) el.innerHTML = val; else el.innerText = val; return true; } return false; };");
  add("  const hide = (id, state) => { const el = document.getElementById(id); if(el) el.classList.toggle('hidden', state); };");

  add("  const setVal = (name, val) => { const el = document.querySelector('[name=\"'+name+'\"]'); if(el && document.activeElement !== el) el.value = val; };");
  add("  const setCb = (id, val, byName=false) => { const el = byName ? document.querySelector('[name=\"'+id+'\"]') : document.getElementById(id); if(el) el.checked = (val === 1 || val === true || val === '1'); };");
  add("  const setRadio = (name, val) => { const el = document.querySelector('[name=\"'+name+'\"][value=\"'+val+'\"]'); if(el) el.checked = true; };");

  add("  if (d.refresh_min !== undefined && !formDirty) {");
  add("    setVal('refresh_min', d.refresh_min);");
  add("    setCb('autoCycle', d.auto_cycle);");
  add("    setVal('screen_int', d.screen_int);");
  add("    setVal('touch_pin', d.touch_pin);");
  add("    setRadio('time_format', d.time_format);");
  
  add("    setCb('autoDetect', d.auto_detect);");
  add("    setVal('latitude', d.latitude);");
  add("    setVal('longitude', d.longitude);");
  add("    setVal('country', d.country);");
  add("    setVal('country_code', d.country_code);");
  add("    setVal('city', d.city);");
  add("    setVal('timezone', d.timezone);");
  
  add("    setCb('nightMode', d.night_mode);");
  add("    setVal('night_start', d.night_start);");
  add("    setVal('night_end', d.night_end);");
  add("    setVal('night_action', d.night_action);");
  add("    setVal('night_dim_start', d.night_dim_start);");
  add("    updateNightAction();");
  
  add("    setCb('showTime', d.show_time);");
  add("    setCb('date_display', d.date_display, true);");

  add("    setCb('showCalendar', d.show_calendar);");
  add("    setRadio('cal_start', d.cal_start);");
  add("    setCb('cal_hol', d.cal_hol, true);");
  add("    setCb('cal_min', d.cal_min, true);");
  
  add("    setCb('showWeather', d.show_weather);");
  add("    setRadio('temp_unit', d.temp_unit);");
  add("    setCb('round_temps', d.round_temps, true);");

  add("    setCb('showAQI', d.show_aqi);");
  add("    setRadio('aqi_type', d.aqi_type);");

  add("    setCb('showPc', d.show_pc);");

  add("    setCb('showStock', d.show_stock);");
  add("    setVal('stock_symbol', d.stock_symbol);");
  
  add("    setCb('stock_fn', d.stock_fn, true);");
  add("    setCb('showCrypto', d.show_crypto);");
  add("    setVal('crypto_id', d.crypto_id);");
  add("    setCb('crypto_fn', d.crypto_fn, true);");
  
  add("    setCb('showCurrency', d.show_currency);");
  add("    setVal('currency_base', d.currency_base);");
  add("    setVal('currency_target', d.currency_target);");
  add("    setVal('currency_multiplier', d.currency_multiplier);");
  add("    setCb('currency_fn', d.currency_fn, true);");

  add("    setCb('showMedia', d.show_media);");

  add("    setCb('showBambu', d.show_bambu);");
  add("    setVal('bambu_ip', d.bambu_ip);");
  add("    setVal('bambu_sn', d.bambu_sn);");
  add("    setVal('bambu_code', d.bambu_code);");

  add("    setCb('hide_empty_pc', d.hide_empty_pc, true);");
  add("    setCb('hide_empty_media', d.hide_empty_media, true);");
  add("    setCb('hide_empty_bambu', d.hide_empty_bambu, true);");

  add("    const mask = d.anim_mask;");
  add("    document.querySelectorAll('.anim-chk').forEach(cb => { cb.checked = (mask & parseInt(cb.value)) !== 0; });");
  add("    const noneBox = document.getElementById('animNone');");
  add("    if (noneBox) { noneBox.checked = (mask === 0); toggleNone(); }");

  add("    if (d.screen_order && !document.querySelector('.dragging')) {");
  add("      const orderArr = d.screen_order.split(',');");
  add("      const list = document.getElementById('sortable-list');");
  add("      if (list) {");
  add("        const items = [...list.querySelectorAll('.sortable-item')];");
  add("        orderArr.forEach(id => { const item = items.find(el => el.getAttribute('data-id') === id); if(item) list.appendChild(item); });");
  add("        updateOrderValue();");
  add("      }");
  add("    }");

  add("    updateVisibility();");
  add("    syncScreenOrder();");
  add("    formDirty = false;");
  add("  }");

  add("  set('time-display', d.time);"); 
  add("  set('preview-time', d.time);");
  add("  set('preview-date', d.date);");

  add("  set('preview-tz', d.timezone);");
  add("  if (d.cal_count !== undefined) { set('preview-hol', d.cal_count > 0 ? d.cal_count : 'No holiday data'); }");

  add("  updateLiveHeader();");

  add("  if (d.temp !== undefined && d.temp !== 'nan') {");
  add("    if (!set('value-temp', d.temp + ' °' + d.temp_unit)) { location.reload(); return; }");
  add("    hide('weather-no-data', true); hide('weather-grid', false);");
  add("    set('value-feels', d.apparent_temperature + ' °' + d.temp_unit);");
  add("    set('value-hum', d.humidity + '%');");
  add("    set('value-wind', d.wind_speed + ' km/h');");
  add("    set('weather-upd', 'Last Update: ' + d.update_time);");
  add("  } else { hide('weather-no-data', false); hide('weather-grid', true); }");

  add("  if (d.aqi !== undefined && d.aqi !== 'nan') {");
  add("    if (!set('value-aqi', d.aqi)) { location.reload(); return; }");
  add("    hide('aqi-no-data', true); hide('aqi-grid', false);");
  add("    const aqiLabel = document.querySelector('#value-aqi + .tile-label'); if(aqiLabel) aqiLabel.innerText = d.aqi_status + ' Index';"); 
  add("    set('value-pm25', d.pm25 + ' <small>µg</small>', true);");
  add("    set('value-pm10', d.pm10 + ' <small>µg</small>', true);");
  add("    set('value-no2', d.no2 + ' <small>µg</small>', true);");
  add("    set('aqi-upd', 'Last Update: ' + d.update_time);");
  add("  } else { hide('aqi-no-data', false); hide('aqi-grid', true); }");

  add("  if (d.crypto_price !== undefined && d.crypto_price !== 'nan') {");
  add("    if (!set('crypto-price', d.crypto_price + '$')) { location.reload(); return; }");
  add("    hide('crypto-no-data', true); hide('crypto-grid', false);");
  add("    set('crypto-change', d.crypto_change + '%');");
  add("    set('crypto-trend-icon', parseFloat(d.crypto_change) >= 0 ? '📈' : '📉');");
  add("    set('crypto-upd', 'Last Update: ' + d.update_time);");
  add("  } else { hide('crypto-no-data', false); hide('crypto-grid', true); }");

  add("  if (d.currency_base_text !== undefined) {");
  add("    if (!set('currency-base-val', d.currency_base_text)) { location.reload(); return; }");
  add("    hide('currency-no-data', true); hide('currency-grid', false);");
  add("    set('currency-target-val', d.currency_target_text);");
  add("    set('currency-upd', 'Last Update: ' + d.update_time);");
  add("  } else { hide('currency-no-data', false); hide('currency-grid', true); }");
  
  add("  if (d.stock_price !== undefined && d.stock_price !== 'nan') {");
  add("    if (!set('stock-price', '$' + d.stock_price)) { location.reload(); return; }");
  add("    hide('stock-no-data', true); hide('stock-grid', false);");
  add("    set('stock-change', d.stock_change + '%');");
  add("    set('stock-trend-icon', parseFloat(d.stock_change) >= 0 ? '📈' : '📉');");
  add("    set('stock-sym', d.stock_symbol + ' Price');"); 
  add("    set('stock-upd', 'Last Update: ' + d.update_time);");
  add("  } else { hide('stock-no-data', false); hide('stock-grid', true); }");

  add("  if (d.pc_cpu !== undefined && d.pc_cpu !== '0.00' && d.pc_cpu !== '0') {");
  add("    if (!set('pc-cpu', Math.round(parseFloat(d.pc_cpu)) + '%')) { location.reload(); return; }");
  add("    hide('pc-no-data', true); hide('pc-grid', false);");
  add("    set('pc-net', Math.round(parseFloat(d.pc_net)) + ' KB/s');");
  add("    set('pc-ram', Math.round(parseFloat(d.pc_ram)) + '%');");
  add("    set('pc-disk', Math.round(parseFloat(d.pc_disk)) + '%');");
  add("  } else { hide('pc-no-data', false); hide('pc-grid', true); }");

  add("  if (d.media_name && d.media_name !== '' && d.media_author && d.media_author !== '') {");
  add("    hide('media-no-data', true); hide('media-grid', false);");
  add("    let s = d.media_status || 'stopped';");
  add("    set('web-media-status', s.charAt(0).toUpperCase() + s.slice(1));");
  add("    set('web-media-name', d.media_name);");
  add("    set('web-media-author', d.media_author);");
  add("    set('web-media-album', d.media_album || 'Unknown');");
  add("  } else { hide('media-no-data', false); hide('media-grid', true); }");

  add("  if (d.bambu_status !== undefined) {");
  add("    hide('bambu-no-data', true); hide('bambu-grid', false);");
  add("    set('bambu-status', d.bambu_status);");
  add("    set('bambu-prog', d.bambu_progress + '% | ' + d.bambu_time + 'm<br><span style=\"font-size:0.9rem\">Layer: ' + d.bambu_layer + '/' + d.bambu_total_layers + '</span>', true);");
  add("    set('bambu-temps', 'Nozzle: ' + parseFloat(d.bambu_nozzle).toFixed(1) + '/' + parseFloat(d.bambu_nozzle_target).toFixed(1) + '<br>Bed: ' + parseFloat(d.bambu_bed).toFixed(1) + '/' + parseFloat(d.bambu_bed_target).toFixed(1), true);");
  add("    set('bambu-fans', 'Part: ' + d.bambu_fan_part + ' | Aux: ' + d.bambu_fan_aux);");
  add("  } else { hide('bambu-no-data', false); hide('bambu-grid', true); }");
  
  add("  if (d.pc_status !== undefined) set('pc-link-status', d.pc_status);");
  add("}).catch(e => console.log('Sync error:', e)); } setInterval(updateData, 15000); updateData();");
  add("</script></div></body></html>");

  if (chunk.length() > 0) {
    server.sendContent(chunk);
  }
  server.sendContent(""); 
}

void WebServerService::handleSave() {
  Config& config = state->config;
  WeatherData& weather = state->weather;
  AirQualityData& aqi = state->aqi;
  CryptoData& crypto = state->crypto;
  CurrencyData& currency = state->currency;
  StockData& stock = state->stock;
  PcStats& pc = state->pc;
  PcMedia& media = state->media;

  // 1. Update Screen Visibility & Master Toggles
  config.auto_detect = server.hasArg("auto_detect");
  config.screen_auto_cycle = server.hasArg("auto_cycle");
  config.night_mode = server.hasArg("night_mode");

  config.show_time = server.hasArg("show_time");
  config.show_calendar = server.hasArg("show_calendar");
  config.show_weather = server.hasArg("show_weather");
  config.show_aqi = server.hasArg("show_aqi");
  config.show_crypto = server.hasArg("show_crypto");
  config.show_pc = server.hasArg("show_pc");
  config.show_currency = server.hasArg("show_currency");
  config.show_stock = server.hasArg("show_stock");
  config.show_media = server.hasArg("show_media");
  config.show_bambu = server.hasArg("show_bambu");

  config.hide_empty_pc = server.hasArg("hide_empty_pc");
  config.hide_empty_media = server.hasArg("hide_empty_media");
  config.hide_empty_bambu = server.hasArg("hide_empty_bambu");

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

  if (config.show_calendar) {
    if (server.hasArg("cal_start")) config.calendar_start_day = server.arg("cal_start");
    config.calendar_show_holidays = server.hasArg("cal_hol");
    config.calendar_minimal = server.hasArg("cal_min");
  }

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
  if (server.hasArg("touch_pin")) {
    String touchPinArg = server.arg("touch_pin");
    touchPinArg.trim();
    if (touchPinArg.length() > 0) config.touch_pin = touchPinArg.toInt();
  }
  if (server.hasArg("anim_mask")) config.anim_mask = server.arg("anim_mask").toInt();
  if (server.hasArg("crypto_id")) config.crypto_id = server.arg("crypto_id").toInt();

  // Night Mode Settings
  if (server.hasArg("night_action")) config.night_action = server.arg("night_action").toInt();
  if (server.hasArg("night_start")) config.night_start = server.arg("night_start");
  if (server.hasArg("night_dim_start")) config.night_dim_start = server.arg("night_dim_start");
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

  if (server.hasArg("bambu_ip")) {
    config.bambu_ip = server.arg("bambu_ip");
  }
  if (server.hasArg("bambu_sn")) {
    config.bambu_sn = server.arg("bambu_sn");
  }
  if (server.hasArg("bambu_code")) {
    config.bambu_code = server.arg("bambu_code");
  }

  if (!config.auto_detect && server.hasArg("city")) {
    config.city = server.arg("city"); 
    config.latitude = server.arg("latitude").toFloat();
    config.longitude = server.arg("longitude").toFloat();
    config.timezone = server.arg("timezone");
    
    if (server.hasArg("country_code")) {
      config.country_code = server.arg("country_code");
      
      for (auto c : allCountries) {
        if (config.country_code == String(c.code)) {
            config.country = c.name;
            break;
        }
      }
    }
  }

  // 3. Forcible Reset of Data (State clearing)

  state->calendar.updated = false;
  state->calendar.count = 0;
  
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

  if (!config.show_media) {
    media.status = "stopped";
    media.name = "";
  }

  if (config.refresh_interval_min <= 0) config.refresh_interval_min = 1; 
  if (config.touch_pin < 0 || config.touch_pin > 21) config.touch_pin = 10;

  if (saveCallback) {
    saveCallback();
  }
  
  server.sendHeader("Location", "/", true);
  server.send(HTTP_REDIRECT, "text/plain", "Settings saved. Redirecting...");
}

void WebServerService::handleUpdate() {
  Config& config = state->config;
  CalendarData& calendar = state->calendar;
  WeatherData& weather = state->weather;
  AirQualityData& aqi = state->aqi;
  CryptoData& crypto = state->crypto;
  CurrencyData& currency = state->currency;
  StockData& stock = state->stock;
  PcStats& pc = state->pc;
  PcMedia& media = state->media;
  BambuData bambu = state->bambu;

  DynamicJsonDocument doc(3072); 
  
  // Global Settings
  doc["device_id"] = config.device_id;
  doc["ip_address"] = config.ip_address;
  doc["refresh_min"] = config.refresh_interval_min;
  doc["auto_cycle"] = config.screen_auto_cycle ? 1 : 0;
  doc["screen_int"] = config.screen_interval_sec;
  doc["touch_pin"] = config.touch_pin;
  doc["anim_mask"] = config.anim_mask;
  doc["time_format"] = config.time_format;
  
  // Location Settings
  doc["auto_detect"] = config.auto_detect ? 1 : 0;
  doc["country"] = config.country;
  doc["country_code"] = config.country_code;
  doc["city"] = config.city;
  doc["latitude"] = config.latitude;
  doc["longitude"] = config.longitude;
  doc["timezone"] = config.timezone;

  // Night Mode Settings
  doc["night_mode"] = config.night_mode ? 1 : 0;
  doc["night_start"] = config.night_start;
  doc["night_end"] = config.night_end;
  doc["night_action"] = config.night_action;
  doc["night_dim_start"] = config.night_dim_start;

  // Screen Toggles
  doc["show_time"] = config.show_time ? 1 : 0;
  doc["date_display"] = config.date_display ? 1 : 0;

  doc["show_calendar"] = config.show_calendar ? 1 : 0;
  doc["cal_start"] = config.calendar_start_day;
  doc["cal_hol"] = config.calendar_show_holidays ? 1 : 0;
  doc["cal_min"] = config.calendar_minimal ? 1 : 0;
  
  doc["show_weather"] = config.show_weather ? 1 : 0;
  doc["temp_unit"] = config.temp_unit;
  doc["round_temps"] = config.round_temps ? 1 : 0;
  
  doc["show_aqi"] = config.show_aqi ? 1 : 0;
  doc["aqi_type"] = config.aqi_type;
  
  doc["show_pc"] = config.show_pc ? 1 : 0;

  doc["show_media"] = config.show_media ? 1 : 0;
  doc["media_status"] = state->media.status;
  doc["media_name"] = state->media.name;
  
  doc["show_stock"] = config.show_stock ? 1 : 0;
  doc["stock_symbol"] = config.stock_symbol;
  doc["stock_fn"] = config.stock_fn ? 1 : 0;
  
  doc["show_crypto"] = config.show_crypto ? 1 : 0;
  doc["crypto_id"] = config.crypto_id;
  doc["crypto_fn"] = config.crypto_fn ? 1 : 0;
  
  doc["show_currency"] = config.show_currency ? 1 : 0;
  doc["currency_base"] = config.currency_base;
  doc["currency_target"] = config.currency_target;
  doc["currency_multiplier"] = config.currency_multiplier;
  doc["currency_fn"] = config.currency_fn ? 1 : 0;

  doc["show_bambu"] = config.show_bambu ? 1 : 0;

  doc["bambu_ip"] = config.bambu_ip;
  doc["bambu_sn"] = config.bambu_sn;
  doc["bambu_code"] = config.bambu_code;

  doc["hide_empty_pc"] = config.hide_empty_pc ? 1 : 0;
  doc["hide_empty_media"] = config.hide_empty_media ? 1 : 0;
  doc["hide_empty_bambu"] = config.hide_empty_bambu ? 1 : 0;

  String orderStr = "";
  for(int i = 0; i < NUM_SCREENS; i++) {
      orderStr += String(config.screen_order[i]);
      if(i < NUM_SCREENS - 1) orderStr += ",";
  }
  doc["screen_order"] = orderStr;
  
  doc["time"] = getCurrentTimeShort(config.time_format);
  doc["date"] = getFullDate();
  doc["cal_count"] = calendar.count;
  doc["update_time"] = weather.update_time;

  if (!isnan(weather.temp)) {
    doc["temp"] = String(weather.temp, 1);
    doc["apparent_temperature"] = String(weather.apparent_temperature, 1);
    doc["humidity"] = String(weather.humidity);
    doc["wind_speed"] = String(weather.wind_speed, 1);
    doc["weather_code"] = weather.weather_code;
  }

  if (!isnan(aqi.pm25) && !isnan(aqi.pm10) && !isnan(aqi.no2)) {
    doc["aqi"] = String(aqi.aqi);
    doc["aqi_status"] = aqi.status;
    doc["pm25"] = String(aqi.pm25, 1);
    doc["pm10"] = String(aqi.pm10, 1);
    doc["no2"] = String(aqi.no2, 1);
  }

  if (!isnan(crypto.price_usd) && crypto.price_usd > 0) {
    doc["crypto_symbol"] = String(crypto.symbol);
    doc["crypto_price"] = String(crypto.price_usd);
    doc["crypto_change"] = String(crypto.percent_change_24h);
  }
  
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

  if (pc.cpu_percent > 0.1) {
    doc["pc_cpu"] = String(pc.cpu_percent);
    doc["pc_net"] = String(pc.net_down_kb);
    doc["pc_ram"] = String(pc.mem_percent);
    doc["pc_disk"] = String(pc.disk_percent);
  }

  if (media.status.length() > 0) {
    doc["media_status"] = media.status;
    doc["media_name"] = media.name;
    doc["media_author"] = media.author;
    doc["media_album"] = media.album;
  }

  if (bambu.status != "SYNCING") {
    doc["bambu_status"] = bambu.status;
    doc["bambu_progress"] = bambu.progress;
    doc["bambu_time"] = bambu.time_left;
    doc["bambu_layer"] = bambu.layer;
    doc["bambu_total_layers"] = bambu.total_layers;
    doc["bambu_nozzle"] = String(bambu.nozzle_temp, 1);
    doc["bambu_nozzle_target"] = String(bambu.nozzle_target, 1);
    doc["bambu_bed"] = String(bambu.bed_temp, 1);
    doc["bambu_bed_target"] = String(bambu.bed_target, 1);
    doc["bambu_fan_part"] = bambu.fan_part;
    doc["bambu_fan_aux"] = bambu.fan_aux;
  }

  String activeId = config.active_pc_id;
  int lastDashSync = activeId.lastIndexOf(':');
  if (lastDashSync > 3) activeId = activeId.substring(0, lastDashSync);

  bool isPcPaired = (activeId != "" && (millis() - pc.last_update < PC_DATA_TIMEOUT_MS));
  doc["pc_status"] = isPcPaired ? ("🔒 Paired to " + activeId) : "";
  
  String jsonResponse;
  serializeJson(doc, jsonResponse);
  
  server.send(HTTP_OK, "application/json", jsonResponse);
}

void WebServerService::handlePcStats() {
  if (!server.hasArg("plain")) {
    server.send(HTTP_BAD_REQUEST, "application/json", "{\"status\":\"error\", \"message\":\"Body not received\"}");
    return;
  }
  
  String body = server.arg("plain");
  
  DynamicJsonDocument doc(1024); 
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    server.send(HTTP_BAD_REQUEST, "application/json", "{\"status\":\"error\", \"message\":\"Invalid JSON\"}");
    return;
  }

  String incoming_pc_id = doc["pc_id"] | "";
  if (incoming_pc_id == "") {
    server.send(HTTP_BAD_REQUEST, "application/json", "{\"status\":\"error\", \"message\":\"Missing PC ID\"}");
    return;
  }

  if (millis() - state->pc.last_update > PC_DATA_TIMEOUT_MS || state->config.active_pc_id == incoming_pc_id || state->config.active_pc_id == "") {
    
    state->config.active_pc_id = incoming_pc_id;
    
    state->pc.cpu_percent = doc["cpu_percent"] | 0.0;
    state->pc.mem_percent = doc["mem_percent"] | 0.0;
    state->pc.disk_percent = doc["disk_percent"] | 0.0;
    state->pc.net_down_kb = doc["net_down_kb"] | 0.0;
    
    state->pc.last_update = millis();
    state->pc.is_wifi = true;

    state->media.status = doc["media_status"] | "stopped";
    state->media.name = doc["media_name"] | "";
    state->media.author = doc["media_author"] | "";
    state->media.album = doc["media_album"] | "";
    state->media.last_update = millis();

    server.send(HTTP_OK, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(HTTP_FORBIDDEN, "application/json", "{\"status\":\"error\", \"message\":\"Device is already paired to another PC\"}");
  }
}
