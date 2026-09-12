#include "PcMonitorService.h"

#include <HardwareSerial.h>

#include "JsonSerializer.h"

bool PcMonitorService::handleSerial(AppState &state) {
    bool configUpdated = false;

    while (Serial.available()) {
        String incoming = Serial.readStringUntil('\n');
        incoming.trim();
        
        if (incoming.length() > 0) {
            if (incoming == "GET_UPDATE") {
                String json = JsonSerializer::buildAppStateJson(state);
                Serial.print("SYS_UPDATE:");
                Serial.println(json);
            } 
            else if (incoming.startsWith("SAVE_CFG:")) {
                if (JsonSerializer::parseConfig(incoming.substring(9).c_str(), state)) {
                    configUpdated = true;
                }
            } 
            else if (incoming.startsWith("{")) {
                parseJson(incoming.c_str(), state);
            }
        }
    }

    unsigned long activeTimeout = state.pc.is_wifi ? WIFI_DATA_TIMEOUT_MS : DATA_TIMEOUT_MS;

    if (millis() - state.pc.last_update > activeTimeout) {
        state.pc.cpu_percent = 0;
        state.pc.net_down_kb = 0;
        state.pc.mem_percent = 0;
        state.pc.disk_percent = 0;
    }

    if (millis() - state.media.last_update > activeTimeout) {
        state.media.status = "stopped";
        state.media.name = "";
        state.media.author = "";
        state.media.album = "";
    }

    return configUpdated;
}

void PcMonitorService::parseJson(const char* jsonString, AppState &state) {
    DynamicJsonDocument doc(1024); 
    DeserializationError error = deserializeJson(doc, jsonString);

    if (!error) {
        state.pc.cpu_percent = doc["cpu_percent"] | 0.0;
        state.pc.mem_percent = doc["mem_percent"] | 0.0;
        state.pc.disk_percent = doc["disk_percent"] | 0.0;
        state.pc.net_down_kb = doc["net_down_kb"] | 0.0;
        
        state.media.status = doc["media_status"] | "stopped";
        state.media.name = doc["media_name"] | "";
        state.media.author = doc["media_author"] | "";
        state.media.album = doc["media_album"] | "";
        
        String incoming_id = doc["pc_id"] | "";
        if (incoming_id != "") {
            state.config.active_pc_id = incoming_id;
        }
        
        unsigned long current_time = millis();
        state.pc.last_update = current_time; 
        state.pc.is_wifi = false;
        state.media.last_update = current_time;
    }
}