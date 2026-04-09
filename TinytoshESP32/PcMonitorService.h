#ifndef PC_MONITOR_SERVICE_H
#define PC_MONITOR_SERVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "structs.h"

class PcMonitorService {
public:
    bool handleSerial(AppState &state);
    const PcStats& getStats() const;

private:
    PcStats currentStats = {0.0, 0.0, 0.0, 0.0};

    static const int JSON_BUF_SIZE = 256;
    char serialBuffer[JSON_BUF_SIZE];
    int bufferIndex = 0;

    void sendUpdateOverSerial(AppState &state);
    void parseJson(const char* jsonString, AppState &state);
    bool parseConfigJson(const char* jsonString, AppState &state);

    const unsigned long DATA_TIMEOUT_MS = 5000;
};

#endif