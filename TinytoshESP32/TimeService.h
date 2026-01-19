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
    String getCurrentTimeShort(String format);
    String getCurrentTime(String format);
    String getFullDate();
    String lookupPosixTimezone(const String& ianaTimezone);

private:
    const char* LOCATION_API_URL = "http://ip-api.com/json/";
    const char* ntpServer = "pool.ntp.org";
    const long  gmtOffset_sec = 0; 
    const int   daylightOffset_sec = 0;
};

#endif