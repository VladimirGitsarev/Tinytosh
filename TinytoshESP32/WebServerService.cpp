#include "WebServerService.h"
#include "DaylightService.h"
#include "TimeService.h"
#include "WeatherService.h"
#include "JsonSerializer.h"
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include "zones.h"

WebServerService::WebServerService(int port, ConfigSaveCallback callback) : 
  server(port), saveCallback(callback) {}

void WebServerService::setAppState(AppState* appState) {
  state = appState;
}

void WebServerService::begin() {
  server.on("/", HTTP_GET, [this](){ this->handleRoot(); }); 
  server.on("/save", HTTP_POST, [this](){ this->handleSave(); });
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
  DaylightData& daylight = state->daylight;
  MoonData& moon = state->moon;
  PcStats& pc = state->pc;
  PcMedia& media = state->media;
  
  bool weatherValid = !isnan(weather.temp);
  bool aqiValid = !isnan(aqi.pm25) && !isnan(aqi.pm10) && !isnan(aqi.no2);
  bool daylightValid = daylight.sunrise_mins != -1;
  bool moonValid = (moon.curphase != "N/A");
  bool pcValid = pc.cpu_percent > 0.1;

  add("<html><head><title>Tinytosh | Web Panel</title>");
  add("<meta name='viewport' content='width=device-width, initial-scale=1'><meta charset='UTF-8'>");
  add("<style>");
  add(":root { --base-bg: #000000; --base-surface: #111111; --base-primary: #ffffff; --base-text: #ffffff; ");
  add("--text-main: var(--base-text); --text-muted: color-mix(in srgb, var(--base-text) 55%, var(--base-bg) 45%); --text-faint: color-mix(in srgb, var(--base-text) 25%, var(--base-bg) 75%); --text-on-primary: var(--base-bg); ");
  add("--surface-main: var(--base-surface); --surface-hover: color-mix(in srgb, var(--base-surface) 92%, var(--base-text) 8%); --surface-active: color-mix(in srgb, var(--base-surface) 85%, var(--base-text) 15%); ");
  add("--border-subtle: color-mix(in srgb, var(--base-surface) 80%, var(--base-text) 20%); --border-strong: color-mix(in srgb, var(--base-primary) 60%, transparent); ");
  add("--primary-main: var(--base-primary); --primary-hover: color-mix(in srgb, var(--base-primary) 80%, var(--base-text) 20%); --primary-glow: color-mix(in srgb, var(--base-primary) 15%, transparent); --shadow-base: color-mix(in srgb, var(--base-bg) 90%, black 10%); ");
  add("--font-sans: system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; --font-tech: 'SF Mono', ui-monospace, 'Cascadia Code', 'Source Code Pro', Menlo, Consolas, monospace; }");
  
  add("* { box-sizing: border-box; } html, body { margin: 0; padding: 0; }");
  add("body { font-family: var(--font-sans); font-weight: 500; background-color: var(--base-bg); color: var(--text-main); padding: 20px; line-height: 1.6; }");
  add(".container { max-width: 800px; margin: 0 auto; }");
  add(".app-header { font-family: var(--font-tech); font-weight: 900; font-size: 2.8rem; color: var(--text-main); text-align: center; padding-bottom: 20px; letter-spacing: 2px; text-shadow: 0 0 20px var(--primary-glow); text-transform: uppercase; }");
  
  add(".mt-0 { margin-top: 0 !important; } .mt-25 { margin-top: 25px; } .mb-20 { margin-bottom: 20px; } .hidden { display: none !important; } hr { border: 0; border-top: 1px solid var(--border-subtle); margin: 20px 0; }");
  add(".panel, .tile, .telemetry-tile, .identity-box, .no-data-tile, select, input, .checkbox-wrapper, button, fieldset, .anim-grid, .sortable-list, .log-container { border-radius: 6px; }");
  
  add(".panel { background: var(--surface-main); border: 1px solid var(--border-subtle); padding: 25px; margin-bottom: 24px; box-shadow: 0 10px 30px var(--shadow-base); }");
  add(".header-panel { border: 1px solid var(--border-strong); box-shadow: 0 0 20px var(--primary-glow); }");
  add(".panel-title { font-family: var(--font-tech); font-weight: 800; color: var(--text-main); font-size: 1.25rem; letter-spacing: -0.5px; text-transform: uppercase; margin-top: 0; margin-bottom: 15px; }");
  
  add(".tile, .telemetry-tile { background: linear-gradient(145deg, var(--surface-hover), var(--surface-main)); border: 1px solid var(--border-subtle); padding: 20px; text-align: center; box-shadow: 0 4px 15px var(--shadow-base); transition: all 0.2s ease; }");
  add(".tile:hover { transform: translateY(-2px); border-color: var(--border-strong); box-shadow: 0 8px 25px var(--primary-glow); }");
  add(".tile-icon { font-size: 2.2rem; margin-bottom: 10px; }");
  add(".tile-label { font-size: 0.75rem; color: var(--text-muted); letter-spacing: 1px; text-transform: uppercase; margin-top: 5px; font-weight: 600; }");
  add(".tile-value { font-family: var(--font-tech); font-weight: 800; font-size: 1.6rem; color: var(--primary-main); line-height: 1.2; text-shadow: 0 0 10px var(--primary-glow); } .date-val { font-size: 1.1rem; }");
  
  add("#time-display { font-family: var(--font-tech); font-weight: 900; font-size: 4.5rem; text-align: center; color: var(--text-main); letter-spacing: -2px; text-shadow: 0 0 20px var(--primary-glow); }");
  add("#location-info { text-align: center; margin: 0; color: var(--text-muted); font-weight: 500; font-size: 1.1rem; }");
  add("#greetings-text { display: none; text-align: center; color: var(--primary-main); font-weight: 600; margin-top: 5px; margin-bottom: 20px; }");
  add(".identity-box { background: var(--surface-active); border: 1px solid var(--border-subtle); padding: 15px; margin-top: 20px; text-align: center; }");
  add(".id-text { font-family: var(--font-tech); font-weight: 800; font-size: 1.2rem; color: var(--text-main); margin-bottom: 4px; } .ip-text { font-family: var(--font-tech); font-size: 0.85rem; color: var(--text-muted); margin-bottom: 8px; }");
  add(".status-badge { font-size: 0.8rem; font-weight: 700; letter-spacing: 1px; text-transform: uppercase; text-align: center; } #status-text { color: var(--text-muted); } #tinytosh-link-status { color: var(--primary-main); }");
  add(".no-data-tile { background: var(--surface-active); border: 1px dashed var(--border-strong); padding: 30px; color: var(--primary-main); text-align: center; font-weight: 600; margin-top: 15px; grid-column: 1 / -1; }");
  
  add(".dashboard-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 15px; margin-top: 20px; } .dashboard-grid > div { min-width: 0; }");
  add(".section-label { font-size: 0.8rem; color: var(--text-muted); margin-bottom: 5px; display: block; letter-spacing: 1px; text-transform: uppercase; font-weight: 700;} label { display: block; margin-top: 15px; font-weight: 600; color: var(--text-main); } .help-text { font-size: 0.85em; color: var(--text-muted); margin-top: 6px; }");
  
  add("select, input[type='text'], input[type='number'], input[type='time'], input[type='color'] { display: block; width: 100% !important; padding: 12px 15px; margin: 8px 0; background-color: var(--surface-active); color: var(--text-main); border: 1px solid var(--border-subtle); font-family: var(--font-tech); font-size: 14px; font-weight: 600; transition: all 0.2s; text-transform: uppercase; }");
  add("select:focus, input:focus { border-color: var(--primary-main); outline: none; box-shadow: 0 0 0 3px var(--primary-glow); } input:disabled, select:disabled { opacity: 0.5; cursor: not-allowed; }");
  add("select { appearance: none; -webkit-appearance: none; background-image: url('data:image/svg+xml;utf8,<svg fill=\"%23ffffff\" height=\"24\" viewBox=\"0 0 24 24\" width=\"24\" xmlns=\"http://www.w3.org/2000/svg\"><path d=\"M7 10l5 5 5-5z\"/></svg>'); background-repeat: no-repeat; background-position: right 12px center; }");
  add("input[type='color'] { padding: 0; cursor: pointer; height: 45px; border: 2px solid var(--border-subtle); } input[type='color']::-webkit-color-swatch-wrapper { padding: 0; } input[type='color']::-webkit-color-swatch { border: none; }");
  
  add("input[type='checkbox'], input[type='radio'] { accent-color: var(--primary-main); cursor: pointer; width: 18px; height: 18px; }");
  add(".checkbox-wrapper { display: flex; align-items: center; margin-bottom: 20px; padding: 15px; background: var(--surface-hover); border: 1px solid var(--border-subtle); } .checkbox-wrapper label { margin-left: 12px; font-weight: 600; color: var(--text-main); font-size: 0.95rem; cursor: pointer; }");
  add(".checkbox-label { display: flex; align-items: center; gap: 10px; margin-top: 12px; cursor: pointer; font-weight: 600; } .radio-group { display: flex; gap: 20px; margin-top: 10px; } .radio-label { display: flex; align-items: center; gap: 8px; cursor: pointer; margin-top: 0; font-weight: 500; }");
  
  add("button { background-color: var(--primary-main); color: var(--text-on-primary); padding: 16px; border: none; cursor: pointer; margin-top: 8px; width: 100%; font-family: var(--font-tech); font-size: 1.1rem; font-weight: 800; text-transform: uppercase; letter-spacing: 1px; transition: all 0.2s ease; box-shadow: 0 4px 15px var(--primary-glow); }");
  add("button:hover { background-color: var(--primary-hover); transform: translateY(-1px); box-shadow: 0 6px 20px var(--primary-glow); } button:active { transform: translateY(1px); } button:disabled { opacity: 0.5; cursor: not-allowed; transform: none; box-shadow: none; }");
  add(".btn-secondary { background-color: var(--surface-active); color: var(--text-main); border: 1px solid var(--border-subtle); box-shadow: none; } .btn-secondary:hover { background-color: var(--primary-main); color: var(--text-on-primary); border-color: var(--primary-main); }");
  
  add("fieldset { border: 1px solid var(--border-strong); padding: 25px; margin-top: 20px; background: var(--surface-hover); } legend { font-family: var(--font-tech); color: var(--primary-main); font-weight: 800; padding: 0 12px; font-size: 0.9rem; text-transform: uppercase; letter-spacing: 1px; }");
  
  add(".anim-label { margin-top: 20px; margin-bottom: 10px; font-weight: 700; display: block; } .anim-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 15px; margin-bottom: 15px; padding: 20px; background: var(--surface-hover); border: 1px solid var(--border-subtle); } .anim-item { display: flex; align-items: center; gap: 8px; cursor: pointer; font-size: 0.95em; margin-top: 0; padding: 5px 0; font-weight: 500; }");
  
  add(".sortable-list { list-style: none; padding: 0; margin: 15px 0 0; border: 1px solid var(--border-subtle); background: var(--surface-main); overflow: hidden; } .sortable-item { background: var(--surface-active); border-bottom: 1px solid var(--border-subtle); padding: 15px 20px; display: flex; align-items: center; gap: 15px; cursor: grab; color: var(--text-main); font-weight: 600; transition: background 0.2s; } .sortable-item:active { cursor: grabbing; } .sortable-item:last-child { border-bottom: none; } .sortable-item.disabled { opacity: 0.4; background: var(--surface-main); }");
  add(".order-ctrl { display: flex; flex-direction: column; margin-right: 15px; gap: 4px; } .move-btn { cursor: pointer; color: var(--text-muted); font-size: 0.7rem; padding: 4px 8px; border: 1px solid var(--border-subtle); border-radius: 4px; background: var(--surface-main); transition: all 0.2s; } .move-btn:hover { color: var(--text-on-primary); background: var(--primary-main); border-color: var(--primary-main); } .sortable-item.disabled .order-ctrl { display: none; }");
  add(".update-footer { text-align: center; font-size: 0.8rem; color: var(--text-muted); margin-top: 20px; font-family: var(--font-tech); }");
  
  add(".multi-row { display: flex; gap: 10px; align-items: flex-end; margin-bottom: 12px; } .multi-row .input-wrapper { flex: 1; min-width: 0; } .multi-row .input-wrapper select { margin: 0; } .btn-remove { width: 45px !important; height: 45px !important; margin: 0 !important; padding: 0 !important; flex-shrink: 0; font-size: 1.5rem !important; display: flex; align-items: center; justify-content: center; background-color: var(--surface-active); color: var(--text-muted); border: 1px solid var(--border-subtle); box-shadow: none; font-family: var(--font-sans); font-weight: 400; } .btn-remove:hover { background-color: var(--primary-main); color: var(--text-on-primary); border-color: var(--primary-main); }");
  add("@media (max-width: 400px) { .dashboard-grid { grid-template-columns: 1fr; } #time-display { font-size: 3.5rem; } }");
  add("</style></head><body><div class='container'>");

  add("<div class='app-header'>Tinytosh</div>");
  add("<div class='panel header-panel'><div id='time-display'>" + TimeService::getCurrentTimeShort(config.time_format) + "</div>"); 
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
  
  add("<div class='panel'><h3 class='panel-title'>Hardware Setup</h3>");
  add("<div class='dashboard-grid mt-0'>");
  
  auto buildPinSelect = [&](String name, String label, int currentValue) {
      String out = "<label class='mt-0'>" + label + ":</label><select name='" + name + "' class='hw-pin'>";
      for (int p = 0; p <= 21; p++) {
          out += "<option value='" + String(p) + "'" + (p == currentValue ? " selected" : "") + ">GPIO " + String(p) + "</option>";
      }
      out += "</select>";
      return out;
  };

  add("  <div>" + buildPinSelect("sda_pin", "I2C SDA Pin", config.sda_pin) + "</div>");
  add("  <div>" + buildPinSelect("scl_pin", "I2C SCL Pin", config.scl_pin) + "</div>");
  add("</div>");
  add(buildPinSelect("touch_pin", "Touch Sensor GPIO Pin", config.touch_pin));
  add("<p class='help-text mt-0'>Reboot Tinytosh to apply any hardware pin changes.</p>");
  add("</div>");

  add("<div class='panel'><h3 class='panel-title'>Global Settings</h3>");
  add("<label class='mt-0'>Color Theme:</label>");
  add("<p class='help-text mt-0'>Customize the 4 base colors to recolor the entire app interface.</p>");
  add("<div class='dashboard-grid mt-0'>");
  add("  <div><label class='mt-0'>App Background:</label><input type='color' name='theme_bg' id='theme_bg' value='" + config.theme_bg + "'></div>");
  add("  <div><label class='mt-0'>Surface / Panels:</label><input type='color' name='theme_card' id='theme_card' value='" + config.theme_card + "'></div>");
  add("  <div><label class='mt-0'>Primary Accent (Highlights):</label><input type='color' name='theme_accent' id='theme_accent' value='" + config.theme_accent + "'></div>");
  add("  <div><label class='mt-0'>Main Text:</label><input type='color' name='theme_text' id='theme_text' value='" + config.theme_text + "'></div>");
  add("</div><hr>");

  add("<label>Data Sync Interval (Mins):</label><input type='number' name='refresh_min' value='" + String(config.refresh_interval_min) + "'>");
  add("<label class='checkbox-label mt-0' style='margin-top: 10px !important;'><input type='checkbox' id='autoCycle' name='auto_cycle' value='1' " + String(config.screen_auto_cycle ? "checked" : "") + "> Cycle Screens Automatically</label>");
  add("<p class='help-text mt-0'>If disabled, screens will only change when you press the button.</p>");
  add("<label>Screen Cycle Interval (Secs):</label><input type='number' id='screenIntInput' name='screen_int' value='" + String(config.screen_interval_sec) + "'>");

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
  
  const char* tzCursor = POSIX_TIMEZONE_MAP;
  while ((tzCursor = strchr(tzCursor, '"')) != nullptr) {
      const char* keyStart = tzCursor + 1;
      const char* keyEnd = strchr(keyStart, '"');
      if (!keyEnd) break;

      const char* colon = keyEnd + 1;
      while (*colon == ' ' || *colon == '\t') colon++;
      if (*colon != ':') {
          tzCursor = keyEnd + 1;
          continue;
      }

      String key;
      key.reserve(keyEnd - keyStart);
      for (const char* p = keyStart; p < keyEnd; p++) key += *p;
      add("<option value='" + key + "'" + (key == config.timezone ? " selected" : "") + ">" + key + "</option>");

      const char* valueStart = strchr(colon, '"');
      if (!valueStart) break;
      const char* valueEnd = strchr(valueStart + 1, '"');
      if (!valueEnd) break;
      tzCursor = valueEnd + 1;
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
      case SCREEN_DAYLIGHT: targetId = "showDaylight"; break;
      case SCREEN_MOON: targetId = "showMoon"; break;
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
              add("<div class='tile'><div class='tile-icon'>🕒</div><div class='tile-value' id='preview-time'>" + TimeService::getCurrentTimeShort(config.time_format) + "</div><div class='tile-label'>Current Time</div></div>");
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
              add("<div class='tile'><div class='tile-icon'>📅</div><div class='tile-value date-val' id='preview-date'>" + TimeService::getFullDate() + "</div><div class='tile-label'>Current Date</div></div>");
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
              add("<div class='tile'><div class='tile-icon' id='icon-temp'>" + WeatherService::getWeatherIcon(weather.weather_code) + "</div><div class='tile-value' id='value-temp'>" + String(weather.temp, 1) + " °" + config.temp_unit + "</div><div class='tile-label'>Temperature</div></div>");
              add("<div class='tile'><div class='tile-icon'>🤒</div><div class='tile-value' id='value-feels'>" + String(weather.apparent_temperature, 1) + " °" + config.temp_unit + "</div><div class='tile-label'>Feels Like</div></div>");
              add("<div class='tile'><div class='tile-icon'>💧</div><div class='tile-value' id='value-hum'>" + String(weather.humidity) + "%</div><div class='tile-label'>Humidity</div></div>");
              add("<div class='tile'><div class='tile-icon'>💨</div><div class='tile-value' id='value-wind'>" + String(weather.wind_speed, 1) + " km/h</div><div class='tile-label'>Wind Speed</div></div>");
              add("</div><div class='update-footer' id='weather-upd'>Last Update: " + weather.update_time + "</div></div>");
              
              add("<label>Temperature Unit:</label><div class='radio-group'>");
              add("<label class='radio-label'><input type='radio' name='temp_unit' value='C' " + String(config.temp_unit == "C" ? "checked" : "") + "> °C</label>");
              add("<label class='radio-label'><input type='radio' name='temp_unit' value='F' " + String(config.temp_unit == "F" ? "checked" : "") + "> °F</label></div>");
              
              add("<label class='checkbox-label'><input type='checkbox' name='round_temps' value='1' " + String(config.round_temps ? "checked" : "") + "> Round Temperature Values</label>");
              add("<label class='checkbox-label'><input type='checkbox' name='weather_hide_bar' value='1' " + String(config.weather_hide_bar ? "checked" : "") + "> Hide Top Bar (Location & Time)</label>");
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
              add("<label class='checkbox-label'><input type='checkbox' name='aqi_hide_bar' value='1' " + String(config.aqi_hide_bar ? "checked" : "") + "> Hide Top Bar (Location & Time)</label>");
              add("</div></div>");
              break;
          }

          case SCREEN_DAYLIGHT: {
              add("<div class='panel' id='panel-" + String(screenId) + "'>");
              add("<label class='checkbox-label mt-0'><input type='checkbox' id='showDaylight' name='show_daylight' value='1' " + String(config.show_daylight ? "checked" : "") + "> Daylight Screen</label>");
              add("<div id='daylightContent' class='collapsible'>");

              if (!daylightValid) {
                  add("<div id='daylight-no-data' class='no-data-tile'>☀️ Daylight data will be available after sync</div><div id='daylight-grid' class='hidden'>");
              } else {
                  add("<div id='daylight-no-data' class='no-data-tile hidden'>☀️ Daylight data will be available after sync</div><div id='daylight-grid'>");
              }
              
              add("<div class='dashboard-grid'>");
              add("<div class='tile'><div class='tile-icon'>🌅</div><div class='tile-value' id='val-sunrise'>" + TimeService::formatMinsFromMidnight(daylight.sunrise_mins, config.time_format) + "</div><div class='tile-label'>Sunrise</div></div>");
              add("<div class='tile'><div class='tile-icon'>🌇</div><div class='tile-value' id='val-sunset'>" + TimeService::formatMinsFromMidnight(daylight.sunset_mins, config.time_format) + "</div><div class='tile-label'>Sunset</div></div>");
              add("<div class='tile'><div class='tile-icon'>☀️</div><div class='tile-value' id='val-noon'>" + TimeService::formatMinsFromMidnight(daylight.noon_mins, config.time_format) + "</div><div class='tile-label'>Solar Noon</div></div>");
              add("<div class='tile'><div class='tile-icon'>⏱️</div><div class='tile-value' id='val-length'>" + TimeService::formatDurationMins(daylight.length_mins) + "</div><div class='tile-label'>Day Length</div></div>");
              add("</div></div>");
              
              add("<label class='checkbox-label'><input type='checkbox' name='daylight_min' value='1' " + String(config.daylight_minimal ? "checked" : "") + "> Minimalistic Mode (Hide timeline)</label>");
              add("</div></div>");
              break;
          }

          case SCREEN_MOON: {
            add("<div class='panel' id='panel-" + String(screenId) + "'>");
            add("<label class='checkbox-label mt-0'><input type='checkbox' id='showMoon' name='show_moon' value='1' " + String(config.show_moon ? "checked" : "") + "> Moon Screen</label>");
            add("<div id='moonContent' class='collapsible'>");
            if (!moonValid) {
              add("<div id='moon-no-data' class='no-data-tile'>  Moon data will be available after sync</div><div id='moon-grid' class='hidden'>");
            } else {
              add("<div id='moon-no-data' class='no-data-tile hidden'>  Moon data will be available after sync</div><div id='moon-grid'>");
            }
            
            add("<div class='dashboard-grid'>");
            add("<div class='tile'><div class='tile-icon'>🌙</div><div class='tile-value' id='val-moon-phase' style='font-size:1.1rem; line-height:1.2;'>" + moon.curphase + "</div><div class='tile-label'>Phase</div></div>");
            add("<div class='tile'><div class='tile-icon'>✨</div><div class='tile-value' id='val-moon-illum'>" + ((moon.fracillum != -1) ? String(moon.fracillum) + "%" : String("--%")) + "</div><div class='tile-label'>Illumination</div></div>");
            add("<div class='tile'><div class='tile-icon'>🌔</div><div class='tile-value' id='val-moon-rise'>" + TimeService::formatMinsFromMidnight(moon.rise_mins, config.time_format) + "</div><div class='tile-label'>Moonrise</div></div>");
            add("<div class='tile'><div class='tile-icon'>🌘</div><div class='tile-value' id='val-moon-set'>" + TimeService::formatMinsFromMidnight(moon.set_mins, config.time_format) + "</div><div class='tile-label'>Moonset</div></div>");
            add("</div></div>");
            add("<label class='checkbox-label'><input type='checkbox' name='moon_min' value='1' " + String(config.moon_minimal ? "checked" : "") + "> Minimalistic Mode (Moon phase only)</label>");
            add("</div></div>");
            break;
          }

          case SCREEN_STOCK: {
              add("<div class='panel' id='panel-" + String(screenId) + "'>");
              add("<label class='checkbox-label mt-0'><input type='checkbox' id='showStock' name='show_stock' value='1' " + String(config.show_stock ? "checked" : "") + "> Stock Tracking Screen</label>");
              add("<div id='stockContent' class='collapsible'>");
              
              add("<div id='stock-no-data' class='no-data-tile'>📈 Stock data will be available after sync</div><div id='stock-grid' class='hidden'>");
              add("<div class='dashboard-grid'>");
              add("<div class='tile'><div class='tile-icon'>📊</div><div class='tile-value' id='stock-price' style='font-size:1.0rem; line-height:1.5;'>--</div><div class='tile-label'>PRICES</div></div>");
              add("<div class='tile'><div class='tile-icon'>📈</div><div class='tile-value' id='stock-change' style='font-size:1.0rem; line-height:1.5;'>--</div><div class='tile-label'>24H CHANGE</div></div>");
              add("</div><div class='update-footer' id='stock-upd'>Last Update: --</div></div>");

              add("<div id='stock-list-container'></div>");
              add("<button type='button' class='btn-blue' onclick='addStockRow()'>+ Add Stock / ETF</button>");
              add("<label class='checkbox-label'><input type='checkbox' name='stock_fn' value='1' " + String(config.stock_fn ? "checked" : "") + "> Display Full Company Name</label>");
              add("</div></div>");
              break;
          }

          case SCREEN_CRYPTO: {
              add("<div class='panel' id='panel-" + String(screenId) + "'>");
              add("<label class='checkbox-label mt-0'><input type='checkbox' id='showCrypto' name='show_crypto' value='1' " + String(config.show_crypto ? "checked" : "") + "> Crypto Tracking Screen</label>");
              add("<div id='cryptoContent' class='collapsible'>");
              
              add("<div id='crypto-no-data' class='no-data-tile'>💰 Crypto data will be available after sync</div><div id='crypto-grid' class='hidden'>");
              add("<div class='dashboard-grid'>");
              add("<div class='tile'><div class='tile-icon'>₿</div><div class='tile-value' id='crypto-price' style='font-size:1.0rem; line-height:1.5;'>--</div><div class='tile-label'>PRICES</div></div>");
              add("<div class='tile'><div class='tile-icon'>📈</div><div class='tile-value' id='crypto-change' style='font-size:1.0rem; line-height:1.5;'>--</div><div class='tile-label'>24H CHANGE</div></div>");
              add("</div><div class='update-footer' id='crypto-upd'>Last Update: --</div></div>");
              
              add("<div id='crypto-list-container'></div>");
              add("<button type='button' class='btn-blue' onclick='addCryptoRow()'>+ Add Cryptocurrency</button>");
              add("<label class='checkbox-label'><input type='checkbox' name='crypto_fn' value='1' " + String(config.crypto_fn ? "checked" : "") + "> Display Full Coin Name</label>");
              add("</div></div>");
              break;
          }

          case SCREEN_CURRENCY: {
              add("<div class='panel' id='panel-" + String(screenId) + "'>");
              add("<label class='checkbox-label mt-0'><input type='checkbox' id='showCurrency' name='show_currency' value='1' " + String(config.show_currency ? "checked" : "") + "> Currency Exchange Screen</label>");
              add("<div id='currencyContent' class='collapsible'>");
              
              add("<div id='currency-no-data' class='no-data-tile'>💱 Currency data will be available after sync</div><div id='currency-grid' class='hidden'>");
              add("<div class='dashboard-grid'>");
              add("<div class='tile'><div class='tile-icon'>💵</div><div class='tile-value' id='currency-base-val' style='font-size:1.0rem; line-height:1.5;'>--</div><div class='tile-label'>BASE</div></div>");
              add("<div class='tile'><div class='tile-icon'>💱</div><div class='tile-value' id='currency-target-val' style='font-size:1.0rem; line-height:1.5;'>--</div><div class='tile-label'>EXCHANGE RATE</div></div>");
              add("</div><div class='update-footer' id='currency-upd'>Last Update: --</div></div>");

              add("<div id='currency-list-container'></div>");
              add("<button type='button' class='btn-blue' onclick='addCurrencyRow()'>+ Add Currency Pair</button>");
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
  add("  var pairs = [['autoDetect','manualFields',true], ['nightMode','nightFields',false], ['showTime', 'timeContent',false], ['showCalendar', 'calendarContent',false], ['showWeather','weatherContent',false], ['showDaylight','daylightContent',false], ['showMoon','moonContent',false], ['showPc','pcContent',false], ['showCrypto','cryptoContent',false], ['showCurrency','currencyContent',false], ['showStock','stockContent',false], ['showAQI','aqiContent',false], ['showMedia','mediaContent',false], ['showBambu','bambuContent',false]];");  
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

  add("function updatePinSelects() {");
  add("  const selects = document.querySelectorAll('.hw-pin');");
  add("  const vals = Array.from(selects).map(s => s.value);");
  add("  selects.forEach(sel => {");
  add("    Array.from(sel.options).forEach(opt => {");
  add("      opt.disabled = vals.includes(opt.value) && opt.value !== sel.value;");
  add("    });");
  add("  });");
  add("}");
  add("document.querySelectorAll('.hw-pin').forEach(s => s.addEventListener('change', updatePinSelects));");
  add("updatePinSelects();");

  add("function updateNightAction() {");
  add("  var action = document.getElementById('nightActionSelect').value;");
  add("  var dimCont = document.getElementById('dimStartContainer');");
  add("  if (action === '3') { dimCont.style.display = 'block'; } else { dimCont.style.display = 'none'; }");
  add("}");
  add("document.getElementById('nightActionSelect').addEventListener('change', updateNightAction);");
  add("updateNightAction();");

  add("window.updateRowControls = function(containerId, maxLimit) { const container = document.getElementById(containerId); if(!container) return; const rows = container.children; const addBtn = container.nextElementSibling; if(addBtn && addBtn.tagName === 'BUTTON') { addBtn.style.display = rows.length >= maxLimit ? 'none' : 'block'; } const removeBtns = container.querySelectorAll('.btn-remove'); removeBtns.forEach(btn => { btn.style.display = rows.length <= 1 ? 'none' : 'flex'; }); };");
  add("window.removeRow = function(btn, containerId) { btn.parentElement.remove(); formDirty = true; updateRowControls(containerId, 5); };");

  add("window.addStockRow = function(val = null) { const container = document.getElementById('stock-list-container'); if (!container || container.children.length >= 5) return; const div = document.createElement('div'); div.className = 'multi-row'; let opts = ''; ");
  for(auto s : topStocks) { add("opts += `<option value='" + String(s.ticker) + "'>" + String(s.name) + " - " + String(s.ticker) + "</option>`;"); }
  add("div.innerHTML = `<div class='input-wrapper'><label class='mt-0'>Track Stock:</label><select name='stock_symbols[]'>${opts}</select></div><button type='button' class='btn-remove' onclick=\"removeRow(this, 'stock-list-container')\">-</button>`; container.appendChild(div); if (val) div.querySelector('select').value = val; formDirty = true; updateRowControls('stock-list-container', 5); };");

  add("window.addCryptoRow = function(val = null) { const container = document.getElementById('crypto-list-container'); if (!container || container.children.length >= 5) return; const div = document.createElement('div'); div.className = 'multi-row'; let opts = ''; ");
  for(auto c : topCoins) { add("opts += `<option value='" + String(c.id) + "'>" + String(c.sym) + "</option>`;"); }
  add("div.innerHTML = `<div class='input-wrapper'><label class='mt-0'>Track Crypto:</label><select name='crypto_ids[]'>${opts}</select></div><button type='button' class='btn-remove' onclick=\"removeRow(this, 'crypto-list-container')\">-</button>`; container.appendChild(div); if (val) div.querySelector('select').value = val; formDirty = true; updateRowControls('crypto-list-container', 5); };");

  add("window.addCurrencyRow = function(bVal = null, tVal = null, mVal = null) { const container = document.getElementById('currency-list-container'); if (!container || container.children.length >= 5) return; const div = document.createElement('div'); div.className = 'multi-row'; let cOpts = ''; ");
  for(auto c : allCurrencies) { 
    String codeStr = String(c.code);
    codeStr.toUpperCase();
    add("cOpts += `<option value='" + String(c.code) + "'>" + codeStr + "</option>`;"); 
  }
  add("div.innerHTML = `<div class='input-wrapper'><label class='mt-0'>Base:</label><select name='currency_bases[]'>${cOpts}</select></div><div class='input-wrapper'><label class='mt-0'>Target:</label><select name='currency_targets[]'>${cOpts}</select></div><div class='input-wrapper'><label class='mt-0'>Mult:</label><select name='currency_multipliers[]'><option value='1'>1</option><option value='10'>10</option><option value='100'>100</option><option value='1000'>1000</option></select></div><button type='button' class='btn-remove' onclick=\"removeRow(this, 'currency-list-container')\">-</button>`; container.appendChild(div); if (bVal) div.querySelector(\"select[name='currency_bases[]']\").value = bVal; if (tVal) div.querySelector(\"select[name='currency_targets[]']\").value = tVal; if (mVal) div.querySelector(\"select[name='currency_multipliers[]']\").value = mVal; formDirty = true; updateRowControls('currency-list-container', 5); };");

  add("['autoDetect', 'nightMode', 'showTime', 'showCalendar', 'showWeather', 'showDaylight', 'showMoon', 'showPc', 'showCrypto', 'showCurrency', 'showStock', 'showAQI', 'showMedia', 'showBambu', 'autoCycle'].forEach(id => { var el=document.getElementById(id); if(el) el.addEventListener('change', updateVisibility); });");
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

  add("function applyLiveTheme() { const root = document.documentElement; root.style.setProperty('--base-bg', document.getElementById('theme_bg').value); root.style.setProperty('--base-surface', document.getElementById('theme_card').value); root.style.setProperty('--base-primary', document.getElementById('theme_accent').value); root.style.setProperty('--base-text', document.getElementById('theme_text').value); }");
  add("['theme_bg', 'theme_card', 'theme_accent', 'theme_text'].forEach(id => { const el = document.getElementById(id); if (el) el.addEventListener('input', applyLiveTheme); });");

  add("function syncScreenOrder() {");
  add("  const items = [...list.querySelectorAll('.sortable-item')];");
  add("  let enabled = [], disabled = [];");
  add("  items.forEach(item => {");
  add("    const targetId = item.getAttribute('data-target');");
  add("    const cb = document.getElementById(targetId);");
  add("    if (cb && cb.checked) { item.classList.remove('disabled'); item.setAttribute('draggable', 'true'); enabled.push(item); }");
  add("    else { item.classList.add('disabled'); item.removeAttribute('draggable'); disabled.push(item); }");
  add("  });");
  add("  list.innerHTML = '';");
  add("  enabled.forEach(el => list.appendChild(el)); disabled.forEach(el => list.appendChild(el));"); 
  add("  updateOrderValue();");
  add("}");

  add("function reorderPhysicalPanels(orderCsv) {");
  add("  const container = document.getElementById('dynamic-panels-container');");
  add("  if (!container || !orderCsv) return;");
  add("  const orderArr = orderCsv.split(',');");
  add("  orderArr.forEach(id => { const panel = document.getElementById('panel-' + id); if (panel) container.appendChild(panel); });");
  add("}");

  add("function updateOrderValue() {");
  add("  const items = [...list.querySelectorAll('.sortable-item')];");
  add("  orderInput.value = items.map(item => item.getAttribute('data-id')).join(',');");
  add("  reorderPhysicalPanels(orderInput.value);");
  add("}");

  add("const panelCheckboxes = ['showTime', 'showCalendar', 'showWeather', 'showAQI', 'showDaylight', 'showMoon', 'showCrypto', 'showCurrency', 'showStock', 'showPc', 'showMedia', 'showBambu'];");
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
  add("  e.preventDefault();");
  add("  let mask = 0; document.querySelectorAll('.anim-chk').forEach(cb => { if(cb.checked) mask += parseInt(cb.value); });");
  add("  const formData = new FormData(e.target);");
  add("  const jsonObj = {};");
  add("  formData.forEach((value, key) => {");
  add("    if (value === 'on') jsonObj[key] = 1;");
  add("    else if (!isNaN(value) && value.trim() !== '') jsonObj[key] = Number(value);");
  add("    else jsonObj[key] = value;");
  add("  });");

  add("  e.target.querySelectorAll('input[type=\"checkbox\"]').forEach(cb => { jsonObj[cb.name] = cb.checked ? 1 : 0; });");
  add("  jsonObj['stock_symbols'] = Array.from(e.target.querySelectorAll('select[name=\"stock_symbols[]\"]')).map(s => s.value);");
  add("  jsonObj['crypto_ids'] = Array.from(e.target.querySelectorAll('select[name=\"crypto_ids[]\"]')).map(s => Number(s.value));");
  add("  jsonObj['currency_bases'] = Array.from(e.target.querySelectorAll('select[name=\"currency_bases[]\"]')).map(s => s.value);");
  add("  jsonObj['currency_targets'] = Array.from(e.target.querySelectorAll('select[name=\"currency_targets[]\"]')).map(s => s.value);");
  add("  jsonObj['currency_multipliers'] = Array.from(e.target.querySelectorAll('select[name=\"currency_multipliers[]\"]')).map(s => Number(s.value));");
  add("  jsonObj['anim_mask'] = mask;");
  add("  jsonObj['screen_order'] = document.getElementById('screenOrderInput').value;");

  add("  const btn = document.querySelector('button[type=\"submit\"]');");
  add("  btn.innerText = '⏳ Saving...';");
  add("  btn.style.opacity = '0.7';");
  add("  btn.disabled = true;");
  
  add("  fetch('/save', { method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify(jsonObj) })");
  add("    .then(r => {");
  add("      if (r.ok) {");
  add("        btn.innerText = '✅ Saved Successfully!';");
  add("        btn.style.backgroundColor = 'var(--text-main)';");
  add("        formDirty = false;");
  add("        setTimeout(updateData, 1000);");
  add("      } else { throw new Error('Backend Error'); }");
  add("    })");
  add("    .catch(e => {");
  add("      btn.innerText = '❌ Failed to Save';");
  add("      btn.style.backgroundColor = 'var(--text-main)';");
  add("    })");
  add("    .finally(() => {");
  add("      setTimeout(() => {");
  add("        btn.innerText = '💾 Save & Apply All Settings';");
  add("        btn.style.backgroundColor = 'var(--primary-main)';");
  add("        btn.style.opacity = '1';");
  add("        btn.disabled = false;");
  add("      }, 3000);");
  add("    });");
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
  add("    setVal('theme_bg', d.theme_bg || '#000000'); setVal('theme_card', d.theme_card || '#111111'); setVal('theme_accent', d.theme_accent || '#ffffff'); setVal('theme_text', d.theme_text || '#ffffff'); applyLiveTheme();");
  add("    setVal('sda_pin', d.sda_pin);");
  add("    setVal('scl_pin', d.scl_pin);");
  add("    setVal('touch_pin', d.touch_pin);");
  add("    updatePinSelects();");
  
  add("    setVal('refresh_min', d.refresh_min);");
  add("    setCb('autoCycle', d.auto_cycle);");
  add("    setVal('screen_int', d.screen_int);");
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
  add("    setCb('weather_hide_bar', d.weather_hide_bar, true);");

  add("    setCb('showAQI', d.show_aqi);");
  add("    setRadio('aqi_type', d.aqi_type);");
  add("    setCb('aqi_hide_bar', d.aqi_hide_bar, true);");

  add("    setCb('showDaylight', d.show_daylight);");
  add("    setCb('daylight_min', d.daylight_min, true);");

  add("    setCb('showMoon', d.show_moon);");

  add("    setCb('moon_min', d.moon_min, true);");

  add("    setCb('showPc', d.show_pc);");

  add("    setCb('showStock', d.show_stock); setCb('stock_fn', d.stock_fn, true);");
  add("    const stCont = document.getElementById('stock-list-container'); if (stCont) { stCont.innerHTML = ''; (d.stock_symbols && d.stock_symbols.length > 0 ? d.stock_symbols : ['AAPL']).forEach(s => window.addStockRow(s)); }");
  add("    setCb('showCrypto', d.show_crypto); setCb('crypto_fn', d.crypto_fn, true);");
  add("    const crCont = document.getElementById('crypto-list-container'); if (crCont) { crCont.innerHTML = ''; (d.crypto_ids && d.crypto_ids.length > 0 ? d.crypto_ids : [90]).forEach(c => window.addCryptoRow(c)); }");
  add("    setCb('showCurrency', d.show_currency); setCb('currency_fn', d.currency_fn, true);");
  add("    const cuCont = document.getElementById('currency-list-container'); if (cuCont) { cuCont.innerHTML = ''; if (d.currency_bases && d.currency_bases.length > 0) { for(let i=0; i<d.currency_bases.length; i++) window.addCurrencyRow(d.currency_bases[i], d.currency_targets[i], d.currency_multipliers[i]); } else { window.addCurrencyRow('usd', 'eur', 1); } }");

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

  add("  if (d.sunrise !== undefined) {");
  add("    hide('daylight-no-data', true); hide('daylight-grid', false);");
  add("    set('val-sunrise', d.sunrise);");
  add("    set('val-sunset', d.sunset);");
  add("    set('val-noon', d.solar_noon);");
  add("    set('val-length', d.day_length);");
  add("  } else { hide('daylight-no-data', false); hide('daylight-grid', true); }");

  add("  if (d.moon_phase !== undefined) {");
  add("    hide('moon-no-data', true); hide('moon-grid', false);");
  add("    set('val-moon-phase', d.moon_phase);");
  add("    set('val-moon-illum', d.moon_illum + '%');");
  add("    set('val-moon-rise', d.moon_rise);");
  add("    set('val-moon-set', d.moon_set);");
  add("  } else { hide('moon-no-data', false); hide('moon-grid', true); }");

  add("  if (d.stock_data && d.stock_data.length > 0) {");
  add("    hide('stock-no-data', true); hide('stock-grid', false); let p='', c='';");
  add("    d.stock_data.forEach(s => { p += s.symbol + ': $' + s.price + '<br>'; c += (parseFloat(s.change) >= 0 ? '+' : '') + s.change + '%<br>'; });");
  add("    set('stock-price', p, true); set('stock-change', c, true); set('stock-upd', 'Last Update: ' + d.update_time);");
  add("  } else { hide('stock-no-data', false); hide('stock-grid', true); }");

  add("  if (d.crypto_data && d.crypto_data.length > 0) {");
  add("    hide('crypto-no-data', true); hide('crypto-grid', false); let p='', c='';");
  add("    d.crypto_data.forEach(s => { p += s.symbol + ': $' + s.price + '<br>'; c += (parseFloat(s.change) >= 0 ? '+' : '') + s.change + '%<br>'; });");
  add("    set('crypto-price', p, true); set('crypto-change', c, true); set('crypto-upd', 'Last Update: ' + d.update_time);");
  add("  } else { hide('crypto-no-data', false); hide('crypto-grid', true); }");

  add("  if (d.currency_data && d.currency_data.length > 0) {");
  add("    hide('currency-no-data', true); hide('currency-grid', false); let b='', t='';");
  add("    d.currency_data.forEach(s => { b += s.base_text + '<br>'; t += s.target_text + '<br>'; });");
  add("    set('currency-base-val', b, true); set('currency-target-val', t, true); set('currency-upd', 'Last Update: ' + d.update_time);");
  add("  } else { hide('currency-no-data', false); hide('currency-grid', true); }");

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
  if (!server.hasArg("plain")) {
    server.send(HTTP_BAD_REQUEST, "text/plain", "Body not received");
    return;
  }
  
  String body = server.arg("plain");
  
  if (JsonSerializer::parseConfig(body.c_str(), *state)) {
    if (saveCallback) saveCallback();
    server.send(HTTP_OK, "text/plain", "Saved");
  } else {
    server.send(HTTP_BAD_REQUEST, "text/plain", "Invalid JSON");
  }
}

void WebServerService::handleUpdate() {
  String jsonResponse = JsonSerializer::buildAppStateJson(*state);
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