#ifndef JSON_SERIALIZER_H
#define JSON_SERIALIZER_H

#include <ArduinoJson.h>
#include "structs.h"

class JsonSerializer {
public:
    static String buildAppStateJson(const AppState& state);
    static String buildConfigJson(const Config& config);
    static bool parseConfig(const char* jsonString, AppState& state);

private:
    static void populateConfigDoc(const Config& config, JsonObject configObj);
};

#endif