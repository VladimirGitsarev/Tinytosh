#ifndef PC_MONITOR_SERVICE_H
#define PC_MONITOR_SERVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "structs.h"

class PcMonitorService {
public:
    void begin();
    bool handleSerial(AppState &state);
    static void reconnectUsb();

private:
    static const int JSON_BUF_SIZE = 256;
    static const unsigned long DATA_TIMEOUT_MS = 3000;
    static const unsigned long WIFI_DATA_TIMEOUT_MS = 10000;
    static const unsigned long USB_RECONNECT_DETACH_MS = 500;
    static const int USB_RX_BUFFER_SIZE = 1024;
    static const int USB_TX_BUFFER_SIZE = 8192;

    PcStats currentStats = {0.0, 0.0, 0.0, 0.0};
    char serialBuffer[JSON_BUF_SIZE];
    int bufferIndex = 0;

    static void parseJson(const char* jsonString, AppState &state);
};

#endif