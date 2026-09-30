#ifndef AIR_QUALITY_SERVICE_H
#define AIR_QUALITY_SERVICE_H

#include "structs.h"

class AirQualityService {
public:
  bool fetchAirQuality(const Config& config, AirQualityData &data);

private:
  static constexpr const char* AIR_QUALITY_API_URL = "https://air-quality-api.open-meteo.com/v1/air-quality";

  static String getAQIDescription(int aqi, bool is_eu);
  static bool aqiValueSelected(const Config& config, const char* key);
};

#endif