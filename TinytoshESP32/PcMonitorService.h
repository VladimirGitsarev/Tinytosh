#ifndef PC_MONITOR_SERVICE_H
#define PC_MONITOR_SERVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "structs.h"

class PcMonitorService {
public:

    void handleSerial(PcStats &stats);
    const PcStats& getStats() const;

private:
    PcStats currentStats = {0.0, 0.0, 0.0, 0.0};

    // Constants for serial reading
    static const int JSON_BUF_SIZE = 256;
    char serialBuffer[JSON_BUF_SIZE];
    int bufferIndex = 0;

    void parseJson(const char* jsonString, PcStats &stats);

    unsigned long lastDataTimestamp = 0; 
    const unsigned long DATA_TIMEOUT_MS = 3000;
};

#endif