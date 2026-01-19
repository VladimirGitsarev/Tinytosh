#ifndef STRUCTS_H
#define STRUCTS_H

#include <Arduino.h>

struct Config {
  bool auto_detect = true;

  float latitude = 0.0;
  float longitude = 0.0;
  String timezone = ""; 
  String city = "";
  String time_format = "24";
  bool date_display = true;

  int crypto_id;

  unsigned long refresh_interval_min = 15;
  int screen_interval_sec = 15;
  bool screen_auto_cycle = true;
  uint16_t anim_mask = 62;

  bool round_temps = true; 
  String temp_unit = "C";

  String aqi_type = "US";
  
  bool show_time = true;
  bool show_weather = true;
  bool show_aqi = true;
  bool show_pc = true;
  bool show_crypto = true;
};

struct WeatherData {
  float temp = NAN;
  float apparent_temperature = NAN; 
  float wind_speed = NAN;
  int humidity = 0;
  int weather_code = -1; 
  bool is_day = NAN;
  String update_time = "N/A";
};

struct AirQualityData {
  int aqi = -1;
  float pm25 = NAN;
  float pm10 = NAN;
  float no2 = NAN;
  String status = "N/A";
};

struct PcStats {
    float cpu_percent = NAN;
    float mem_percent = NAN;
    // float cpu_temp = NAN;
    float disk_percent = NAN;
    float net_down_kb = NAN;
};

struct CryptoData {
    String name;
    String symbol;
    float price_usd;
    float percent_change_24h;
    bool updated = false;
};

struct CoinOption { 
    int id; 
    const char* sym; 
};

inline constexpr CoinOption topCoins[] = {
    {90, "BTC"}, {80, "ETH"}, {518, "USDT"}, {48543, "SOL"}, {2710, "BNB"},
    {58, "XRP"}, {33224, "USDC"}, {2, "DOGE"}, {257, "ADA"}, {2713, "TRX"},
    {44857, "AVAX"}, {44800, "SHIB"}, {45131, "DOT"}, {2738, "LINK"}, {51334, "TON"},
    {33536, "MATIC"}, {33234, "WBTC"}, {2321, "BCH"}, {1, "LTC"}, {47305, "NEAR"},
    {44265, "UNI"}, {33285, "DAI"}, {51469, "APT"}, {51811, "PEPE"}, {118, "ETC"},
    {47214, "ICP"}, {32703, "LEO"}, {28, "XMR"}, {172, "XLM"}, {29854, "OKB"}
};

enum ScreenType {
  SCREEN_TIME,
  SCREEN_WEATHER,
  SCREEN_AIR_QUALITY,
  SCREEN_CRYPTO,
  SCREEN_PC_MONITOR,
  NUM_SCREENS
};

enum AnimType {
  ANIM_NONE,
  ANIM_SLIDE_HORIZONTAL,
  ANIM_SLIDE_VERTICAL,
  ANIM_DISSOLVE,
  ANIM_CURTAIN,
  ANIM_BLINDS,
  ANIM_RANDOM
};

#endif