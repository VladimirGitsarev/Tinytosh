#include "IssService.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>

IssService::IssService() {}

bool IssService::fetchPosition(const String& units, IssData& sample) {
  HTTPClient http;
  String url = String(ISS_API_URL) + "?units=" + units;
  Serial.println("IssService: Fetching ISS position -> " + url);

  http.setReuse(false);
  http.begin(url);
  http.setConnectTimeout(5000);
  http.setTimeout(5000);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("IssService: HTTP GET failed, code: %d\n", httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument filter(128);
  filter["latitude"] = true;
  filter["longitude"] = true;
  filter["altitude"] = true;
  filter["velocity"] = true;
  filter["visibility"] = true;
  filter["timestamp"] = true;

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
  if (error || !doc.containsKey("latitude")) {
    Serial.printf("IssService: JSON parsing failed: %s\n", error ? error.c_str() : "no position");
    return false;
  }

  sample.latitude = doc["latitude"].as<float>();
  sample.longitude = doc["longitude"].as<float>();
  sample.altitude = doc["altitude"].as<float>();
  sample.velocity = doc["velocity"].as<float>();
  sample.is_daylight = (doc["visibility"].as<String>() == "daylight");
  sample.timestamp = doc["timestamp"].as<long>();
  sample.units = units;
  return true;
}

bool IssService::fetchIss(const Config& config, IssData& data, bool resolveDirection) {
  String units = (config.iss_units == "miles") ? "miles" : "kilometers";

  IssData fresh = data;

  if (!fetchPosition(units, fresh)) return false;

  long sampleGap = fresh.timestamp - data.timestamp;
  if (isValid(data) && sampleGap > 0 && sampleGap <= MAX_DIRECTION_SAMPLE_GAP_SEC) {
    fresh.prev_latitude = data.latitude;
    fresh.prev_timestamp = data.timestamp;
  } else if (resolveDirection) {
    Serial.println("IssService: No recent previous sample, fetching a second position to determine direction...");
    IssData first = fresh;
    for (int attempt = 0; attempt < DIRECTION_FETCH_ATTEMPTS; attempt++) {
      if (!fetchPosition(units, fresh)) break;
      if (fresh.timestamp > first.timestamp) break;
      Serial.println("IssService: Second sample has the same timestamp, retrying in 1s...");
      delay(1000);
    }

    if (fresh.timestamp > first.timestamp) {
      fresh.prev_latitude = first.latitude;
      fresh.prev_timestamp = first.timestamp;
      Serial.printf("IssService: Direction resolved, heading %s (%.4f -> %.4f over %lds)\n",
                    fresh.latitude >= first.latitude ? "north" : "south", first.latitude, fresh.latitude, fresh.timestamp - first.timestamp);
    } else {
      fresh = first;
      fresh.prev_latitude = NAN;
      Serial.println("IssService: Couldn't determine direction, orbit line will appear after the next sync.");
    }
  } else {
    fresh.prev_latitude = NAN;
  }

  String coordsUrl = String(COORDINATES_API_URL) + String(fresh.latitude, 4) + "," + String(fresh.longitude, 4);
  Serial.println("IssService: Fetching country below -> " + coordsUrl);

  HTTPClient coordsHttp;
  coordsHttp.setReuse(false);
  coordsHttp.begin(coordsUrl);
  coordsHttp.setConnectTimeout(5000);
  coordsHttp.setTimeout(5000);

  int coordsCode = coordsHttp.GET();
  if (coordsCode == HTTP_CODE_OK) {
    String coordsPayload = coordsHttp.getString();
    DynamicJsonDocument coordsFilter(64);
    coordsFilter["country_code"] = true;

    DynamicJsonDocument coordsDoc(128);
    if (!deserializeJson(coordsDoc, coordsPayload, DeserializationOption::Filter(coordsFilter))) {
      fresh.country_code = coordsDoc["country_code"].as<String>();
    }
  } else {
    Serial.printf("IssService: Country lookup failed, code: %d\n", coordsCode);
  }
  coordsHttp.end();

  fresh.fetched_at_ms = millis();
  data = fresh;

  Serial.printf("IssService: Success! Lat: %.2f, Lon: %.2f, Alt: %.0f, Vel: %.0f (%s), Over: %s\n",
                data.latitude, data.longitude, data.altitude, data.velocity, units.c_str(), getLocationName(data).c_str());
  return true;
}

bool IssService::predictPosition(const IssData& data, float minutesAhead, float& lat, float& lon) {
  lat = data.latitude;
  lon = data.longitude;

  long sampleGap = data.timestamp - data.prev_timestamp;
  if (isnan(data.prev_latitude) || sampleGap <= 0 || sampleGap > MAX_DIRECTION_SAMPLE_GAP_SEC) return false;

  bool ascending = data.latitude >= data.prev_latitude;
  float inc = radians(ORBIT_INCLINATION_DEG);
  float s = constrain(sinf(radians(data.latitude)) / sinf(inc), -1.0f, 1.0f);
  float u0 = ascending ? asinf(s) : (PI - asinf(s));
  float u = u0 + TWO_PI * minutesAhead / ORBIT_PERIOD_MIN;

  float lambda0 = atan2f(cosf(inc) * sinf(u0), cosf(u0));
  float lambdaDiff = atan2f(cosf(inc) * sinf(u), cosf(u)) - lambda0;
  lambdaDiff += TWO_PI * roundf(((u - u0) - lambdaDiff) / TWO_PI);

  lat = degrees(asinf(sinf(inc) * sinf(u)));
  lon = data.longitude + degrees(lambdaDiff) - EARTH_ROTATION_DEG_PER_MIN * minutesAhead;
  lon = fmodf(lon + 180.0f, 360.0f);
  if (lon < 0) lon += 360.0f;
  lon -= 180.0f;
  return true;
}

bool IssService::getPassWindow(const IssData& data, float nowMin, float& startMin, float& endMin) {
  float lat, lon, prevLon;
  if (!predictPosition(data, nowMin, lat, prevLon)) return false;

  startMin = nowMin;
  for (float t = nowMin - PASS_STEP_MIN; t > nowMin - MAX_PASS_MIN; t -= PASS_STEP_MIN) {
    predictPosition(data, t, lat, lon);
    if (fabsf(lon - prevLon) > 180.0f) break;
    startMin = t;
    prevLon = lon;
  }

  predictPosition(data, nowMin, lat, prevLon);
  endMin = nowMin;
  for (float t = nowMin + PASS_STEP_MIN; t < nowMin + MAX_PASS_MIN; t += PASS_STEP_MIN) {
    predictPosition(data, t, lat, lon);
    if (fabsf(lon - prevLon) > 180.0f) break;
    endMin = t;
    prevLon = lon;
  }
  return true;
}

void IssService::getCurrentPosition(const IssData& data, float& lat, float& lon) {
  float elapsedMin = (millis() - data.fetched_at_ms) / 60000.0f;
  predictPosition(data, min(elapsedMin, MAX_PREDICTION_MIN), lat, lon);
}

float IssService::distanceFrom(const IssData& data, float lat, float lon) {
  float issLat, issLon;
  getCurrentPosition(data, issLat, issLon);

  float dLat = radians(issLat - lat);
  float dLon = radians(issLon - lon);
  float a = sinf(dLat / 2) * sinf(dLat / 2) + cosf(radians(lat)) * cosf(radians(issLat)) * sinf(dLon / 2) * sinf(dLon / 2);
  float earthRadius = (data.units == "miles") ? 3958.8f : 6371.0f;
  return earthRadius * 2 * atan2f(sqrtf(a), sqrtf(1 - a));
}

String IssService::getLocationName(const IssData& data) {
  if (data.country_code.length() == 2 && data.country_code != "??") {
    for (const auto& country : allCountries) {
      if (data.country_code.equalsIgnoreCase(country.code)) return country.name;
    }
    return data.country_code;
  }
  if (data.country_code == "??") return getOceanName(data.latitude, data.longitude);
  return "";
}

String IssService::getOceanName(float lat, float lon) {
  if (lat > 66) return "Arctic Ocean";
  if (lat < -60) return "Southern Ocean";
  if (lat > 30 && lat < 46 && lon > -6 && lon < 36) return "Mediterranean Sea";
  if (lat > 40.5 && lat < 47 && lon > 27.5 && lon < 42) return "Black Sea";
  if (lat > 36 && lat < 47.5 && lon > 46.5 && lon < 55) return "Caspian Sea";
  if (lat > 12 && lat < 30 && lon > 32 && lon < 44) return "Red Sea";
  if (lat < 30 && lon > 20 && (lon < 100 || (lat < -10 && lon < 147))) return "Indian Ocean";
  bool atlantic = (lon > -70 && lon < 20) || (lat > 8 && lon > -98 && lon <= -70 && !(lat < 18 && lon < -83));
  return atlantic ? "Atlantic Ocean" : "Pacific Ocean";
}
