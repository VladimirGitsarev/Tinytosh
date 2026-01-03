#include <HardwareSerial.h>
#include "PcMonitorService.h"


void PcMonitorService::handleSerial(PcStats &stats) {
    // 1. Process incoming Serial data
    while (Serial.available()) {
        char incomingChar = Serial.read();
        if (incomingChar == '\n' || incomingChar == '\r') {
            if (bufferIndex > 0) {
                serialBuffer[bufferIndex] = '\0';
                parseJson(serialBuffer, stats); 
            }
            bufferIndex = 0;
        } else if (bufferIndex < 256 - 1) {
            if (incomingChar > 31) serialBuffer[bufferIndex++] = incomingChar;
        }
    }

    // 2. Check for Timeout (Heartbeat)
    if (millis() - lastDataTimestamp > DATA_TIMEOUT_MS) {
        stats.cpu_percent = 0;
        stats.net_down_kb = 0;
        stats.mem_percent = 0;
        stats.disk_percent = 0;
    }
}

void PcMonitorService::parseJson(const char* jsonString, PcStats &stats) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, jsonString);
    
    if (error) {
        return;
    }

    lastDataTimestamp = millis();

    stats.cpu_percent = doc["cpu_percent"] | 0.0;
    stats.net_down_kb = doc["net_down_kb"] | 0.0;
    stats.mem_percent = doc["mem_percent"] | 0.0;
    stats.disk_percent = doc["disk_percent"] | 0.0;
}

const PcStats& PcMonitorService::getStats() const {
    return currentStats;
}