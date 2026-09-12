#ifndef CALENDAR_SERVICE_H
#define CALENDAR_SERVICE_H

#include <Arduino.h>

#include "structs.h"

class CalendarService {
public:
    CalendarService();
    void fetchHolidays(const String& countryCode, CalendarData& data);

private:
    static constexpr const char* API_BASE_URL = "https://date.nager.at/api/v3/PublicHolidays/";
};

#endif