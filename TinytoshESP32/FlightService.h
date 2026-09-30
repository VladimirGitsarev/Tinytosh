#ifndef FLIGHT_SERVICE_H
#define FLIGHT_SERVICE_H

#include <ArduinoJson.h>

#include "structs.h"

class FlightService {
public:
    bool fetchFlights(const Config& config, FlightData& data);

private:
    static constexpr const char* ADSBLOL_BASE_URL = "https://api.adsb.lol/v2";
    static constexpr const char* ADSBLOL_ROUTE_URL = "https://api.adsb.lol/api/0/routeset";
    static constexpr const char* ADSBDB_AIRCRAFT_URL = "https://api.adsbdb.com/v0/aircraft/";

    static constexpr const char* USER_AGENT = "Tinytosh/1.0";
    static constexpr const char* HEADER_JSON = "application/json";
    static constexpr const char* HEADER_REFERER = "https://api.adsb.lol/docs";

    static constexpr float EARTH_RADIUS_KM = 6371.0;
    static constexpr int HTTP_TIMEOUT_MS = 10000;

    bool fetchNearbyAircraft(const Config& config, FlightAircraft* out, int maxOut, int& outCount, int& outTotalCount);
    void fetchRoutes(FlightAircraft* aircraft, int count, String* outOriginCity, String* outDestCity);
    void fetchAircraftDetails(const String& icao24, String& outManufacturer, String& outType);

    static float haversineKm(float lat1, float lon1, float lat2, float lon2);
    static float parseAltitudeFt(JsonVariantConst v);
};

#endif
