#ifndef DAYLIGHT_SERVICE_H
#define DAYLIGHT_SERVICE_H

#include <Arduino.h>

#include "structs.h"

class DaylightService {
public:
    DaylightService();
    bool fetchDaylight(const Config& config, DaylightData& data);

    static String formatTime(int mins, String format, bool show_ampm = true);
    static String formatDuration(int mins);

private:
    static constexpr const char* DAYLIGHT_API_BASE = "https://api.sunrise-sunset.org/json";

    static int parseTime(String apiTime);
    static int parseDuration(String apiDuration);
};

#endif