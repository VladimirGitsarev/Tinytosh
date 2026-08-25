#ifndef MOON_SERVICE_H
#define MOON_SERVICE_H

#include "structs.h"
#include <Arduino.h>
#include <HTTPClient.h>

class MoonService {
public:
  MoonService();
  bool fetchMoon(const Config& config, MoonData& data);

private:
  static constexpr const char* MOON_API_BASE = "https://aa.usno.navy.mil/api/rstt/oneday";
};

#endif