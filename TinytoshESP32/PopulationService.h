#ifndef POPULATION_SERVICE_H
#define POPULATION_SERVICE_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <time.h>
#include "structs.h"

class PopulationService {
public:
  PopulationService();
  bool fetchPopulation(const Config& config, PopulationData& data);
  
  static long long getLivePopulation(long long basePop, double growth, int year);

private:
  static constexpr const char* POPULATION_API_BASE = "https://api.worldbank.org/v2/country/";
  
  bool fetchIndicator(String countryCode, String indicator, long long &valInt, double &valFloat, int &year);
};

#endif