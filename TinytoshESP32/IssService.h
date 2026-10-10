#ifndef ISS_SERVICE_H
#define ISS_SERVICE_H

#include <Arduino.h>

#include "structs.h"

class IssService {
public:
  IssService();
  // resolveDirection: when there's no recent previous sample, fetch a second position straight away
  // so the direction of travel (and the orbit line) is known immediately
  bool fetchIss(const Config& config, IssData& data, bool resolveDirection = false);

  // Ground position `minutesAhead` minutes after the last sample (negative = in the past).
  // Returns false when the direction of travel isn't known yet (needs two samples).
  static bool predictPosition(const IssData& data, float minutesAhead, float& lat, float& lon);

  // Time window (minutes relative to the last sample) of the current left-to-right pass across the map,
  // i.e. between the two moments the ground track wraps around the 180° meridian
  static bool getPassWindow(const IssData& data, float nowMin, float& startMin, float& endMin);
  static constexpr float PASS_STEP_MIN = 0.5f;

  // Live position: the last sample advanced by the time elapsed since it was fetched
  static void getCurrentPosition(const IssData& data, float& lat, float& lon);

  // Great-circle distance from the given point to the ISS ground position, in the data's units
  static float distanceFrom(const IssData& data, float lat, float lon);

  // Country name for the ISS ground position, or a rough ocean/sea name when over water
  static String getLocationName(const IssData& data);

  static bool isValid(const IssData& data) { return !isnan(data.latitude); }

private:
  static constexpr const char* ISS_API_URL = "https://api.wheretheiss.at/v1/satellites/25544";
  static constexpr const char* COORDINATES_API_URL = "https://api.wheretheiss.at/v1/coordinates/";

  static constexpr float ORBIT_INCLINATION_DEG = 51.64f;
  static constexpr float ORBIT_PERIOD_MIN = 92.68f;
  static constexpr float EARTH_ROTATION_DEG_PER_MIN = 360.0f / 1436.07f;
  static constexpr float MAX_PREDICTION_MIN = 15.0f;
  static constexpr float MAX_PASS_MIN = 120.0f;
  static constexpr long MAX_DIRECTION_SAMPLE_GAP_SEC = 900;

  static const int DIRECTION_FETCH_ATTEMPTS = 3;

  static bool fetchPosition(const String& units, IssData& sample);
  static String getOceanName(float lat, float lon);
};

#endif
