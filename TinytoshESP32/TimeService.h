#ifndef TIME_SERVICE_H
#define TIME_SERVICE_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <time.h> 
#include "structs.h"

class TimeService {
public:
    TimeService();
    void syncNTP(const String& ianaTimezone);
    bool fetchLocationData(Config& config);
    static String getCurrentTimeShort(String format);
    static String getCurrentTime(String format);
    static String getFullDate();
    static String formatMinsFromMidnight(int mins, String format, bool show_ampm = true);
    static String formatDurationMins(int mins);
    static int parseTimeToMinsFromMidnight(String apiTime, String format = "12");
    static int parseDurationToMins(String apiDuration);
    static String lookupPosixTimezone(const String& ianaTimezone);

private:
    static constexpr const char* LOCATION_API_URL = "http://ip-api.com/json/";
    static constexpr const char* NTP_SERVER = "pool.ntp.org";
    const long  gmtOffset_sec = 0; 
    const int   daylightOffset_sec = 0;
};

#endif