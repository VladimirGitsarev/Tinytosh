#ifndef STRUCTS_H
#define STRUCTS_H

#include <Arduino.h>

enum ScreenType {
  SCREEN_TIME,
  SCREEN_WEATHER,
  SCREEN_AIR_QUALITY,
  SCREEN_CRYPTO,
  SCREEN_CURRENCY,
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

struct Config {
  // Global Settings
  bool auto_detect = true;
  float latitude = 0.0;
  float longitude = 0.0;
  String timezone = ""; 
  String city = "";
  String time_format = "24";
  bool date_display = true;
  unsigned long refresh_interval_min = 15;

  // Screens Settings
  bool screen_auto_cycle = true;
  int screen_interval_sec = 15;

  bool show_time = true;
  bool show_weather = true;
  bool show_aqi = true;
  bool show_crypto = true;
  bool show_currency = true;
  bool show_pc = true;

  // Weather & AQI Settings
  bool round_temps = true; 
  String temp_unit = "C";
  String aqi_type = "US";

  // Crypto & Currency Settings
  int crypto_id;
  String currency_base = "usd";
  String currency_target = "eur";
  int currency_multiplier = 1;

  // Animation Settings
  uint16_t anim_mask = 62;
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

struct CurrencyData {
    String base;
    String target;
    float rate;
    String date;
    bool updated = false;
};

struct CoinOption { 
    int id; 
    const char* sym; 
};

struct CurrencyOption {
    const char* code;
    const char* name;
};

inline constexpr CoinOption topCoins[] = {
    // Top 10 & Majors
    {90, "BTC"}, {80, "ETH"}, {518, "USDT"}, {2710, "BNB"}, {48543, "SOL"},
    {58, "XRP"}, {33224, "USDC"}, {257, "ADA"}, {44857, "AVAX"}, {2, "DOGE"},
    
    // Top 20 & High Caps
    {45131, "DOT"}, {2713, "TRX"}, {2738, "LINK"}, {33536, "MATIC"}, {51334, "TON"},
    {44800, "SHIB"}, {1, "LTC"}, {2321, "BCH"}, {33234, "WBTC"}, {44265, "UNI"},
    
    // Top 50 Prominent Projects
    {28557, "ATOM"}, {47305, "NEAR"}, {47214, "ICP"}, {51469, "APT"}, {51811, "PEPE"},
    {172, "XLM"}, {29854, "OKB"}, {118, "ETC"}, {28, "XMR"}, {32703, "LEO"},
    {45219, "FIL"}, {33503, "HBAR"}, {51745, "ARB"}, {2741, "VET"}, {2816, "MKR"},
    {42564, "CRO"}, {33022, "QNT"}, {33177, "ALGO"}, {46427, "GRT"}, {45088, "AAVE"},
    
    // DeFi, Gaming & Layer 2s
    {44926, "STX"}, {28014, "SNX"}, {2679, "EOS"}, {46087, "EGLD"}, {45224, "SAND"},
    {28318, "THETA"}, {2748, "MANA"}, {2742, "XTZ"}, {46990, "MINA"}, {33309, "FTM"},
    {44365, "KAVA"}, {1376, "NEO"}, {46481, "FLOW"}, {32785, "CHZ"}, {44256, "KLAY"},
    {32729, "RPL"}, {45435, "CRV"}, {46682, "GALA"}, {44866, "COMP"}, {2770, "IOTA"},
    
    // Additional Stablecoins & Ecosystem Tokens
    {33285, "DAI"}, {33814, "PAXG"}, {32684, "BUSD"}, {33282, "TUSD"}, {45204, "FRAX"},
    {44082, "USDP"}, {33263, "ENJ"}, {33190, "BAT"}, {2734, "ZEC"}, {2740, "DASH"},
    {46580, "LDO"}, {51717, "OP"}, {51859, "SUI"}, {51608, "BLUR"}, {51381, "GMX"}
};

inline constexpr CurrencyOption allCurrencies[] = {
    {"aed", "United Arab Emirates Dirham"}, {"afn", "Afghan Afghani"}, {"all", "Albanian Lek"},
    {"amd", "Armenian Dram"}, {"ang", "Netherlands Antillean Guilder"}, {"aoa", "Angolan Kwanza"},
    {"ars", "Argentine Peso"}, {"aud", "Australian Dollar"}, {"awg", "Aruban Florin"},
    {"azn", "Azerbaijani Manat"}, {"bam", "Bosnia-Herzegovina Convertible Mark"}, {"bbd", "Barbadian Dollar"},
    {"bdt", "Bangladeshi Taka"}, {"bgn", "Bulgarian Lev"}, {"bhd", "Bahraini Dinar"},
    {"bif", "Burundian Franc"}, {"bmd", "Bermudan Dollar"}, {"bnd", "Brunei Dollar"},
    {"bob", "Bolivian Boliviano"}, {"brl", "Brazilian Real"}, {"bsd", "Bahamian Dollar"},
    {"btn", "Bhutanese Ngultrum"}, {"bwp", "Botswanan Pula"}, {"byn", "New Belarusian Ruble"},
    {"bzd", "Belize Dollar"}, {"cad", "Canadian Dollar"}, {"cdf", "Congolese Franc"},
    {"chf", "Swiss Franc"}, {"clp", "Chilean Peso"}, {"cny", "Chinese Yuan"},
    {"cop", "Colombian Peso"}, {"crc", "Costa Rican Colón"}, {"cup", "Cuban Peso"},
    {"cve", "Cape Verdean Escudo"}, {"czk", "Czech Republic Koruna"}, {"djf", "Djiboutian Franc"},
    {"dkk", "Danish Krone"}, {"dop", "Dominican Peso"}, {"dzd", "Algerian Dinar"},
    {"egp", "Egyptian Pound"}, {"ern", "Eritrean Nakfa"}, {"etb", "Ethiopian Birr"},
    {"eur", "Euro"}, {"fjd", "Fijian Dollar"}, {"fkp", "Falkland Islands Pound"},
    {"gbp", "British Pound Sterling"}, {"gel", "Georgian Lari"}, {"ghs", "Ghanaian Cedi"},
    {"gip", "Gibraltar Pound"}, {"gmd", "Gambian Dalasi"}, {"gnf", "Guinean Franc"},
    {"gtq", "Guatemalan Quetzal"}, {"gyd", "Guyanaese Dollar"}, {"hkd", "Hong Kong Dollar"},
    {"hnl", "Honduran Lempira"}, {"htg", "Haitian Gourde"}, {"huf", "Hungarian Forint"},
    {"idr", "Indonesian Rupiah"}, {"ils", "Israeli New Sheqel"}, {"inr", "Indian Rupee"},
    {"iqd", "Iraqi Dinar"}, {"irr", "Iranian Rial"}, {"isk", "Icelandic Króna"},
    {"jmd", "Jamaican Dollar"}, {"jod", "Jordanian Dinar"}, {"jpy", "Japanese Yen"},
    {"kes", "Kenyan Shilling"}, {"kgs", "Kyrgystani Som"}, {"khr", "Cambodian Riel"},
    {"kmf", "Comorian Franc"}, {"kpw", "North Korean Won"}, {"krw", "South Korean Won"},
    {"kwd", "Kuwaiti Dinar"}, {"kyd", "Cayman Islands Dollar"}, {"kzt", "Kazakhstani Tenge"},
    {"lak", "Laotian Kip"}, {"lbp", "Lebanese Pound"}, {"lkr", "Sri Lankan Rupee"},
    {"lrd", "Liberian Dollar"}, {"lsl", "Lesotho Loti"}, {"lyd", "Libyan Dinar"},
    {"mad", "Moroccan Dirham"}, {"mdl", "Moldovan Leu"}, {"mga", "Malagasy Ariary"},
    {"mkd", "Macedonian Denar"}, {"mmk", "Myanma Kyat"}, {"mnt", "Mongolian Tugrik"},
    {"mop", "Macanese Pataca"}, {"mru", "Mauritanian Ouguiya"}, {"mur", "Mauritian Rupee"},
    {"mvr", "Maldivian Rufiyaa"}, {"mwk", "Malawian Kwacha"}, {"mxn", "Mexican Peso"},
    {"myr", "Malaysian Ringgit"}, {"mzn", "Mozambican Metical"}, {"nad", "Namibian Dollar"},
    {"ngn", "Nigerian Naira"}, {"nio", "Nicaraguan Córdoba"}, {"nok", "Norwegian Krone"},
    {"npr", "Nepalese Rupee"}, {"nzd", "New Zealand Dollar"}, {"omr", "Omani Rial"},
    {"pab", "Panamanian Balboa"}, {"pen", "Peruvian Nuevo Sol"}, {"pgk", "Papua New Guinean Kina"},
    {"php", "Philippine Peso"}, {"pkr", "Pakistani Rupee"}, {"pln", "Polish Zloty"},
    {"pyg", "Paraguayan Guarani"}, {"qar", "Qatari Rial"}, {"ron", "Romanian Leu"},
    {"rsd", "Serbian Dinar"}, {"rub", "Russian Ruble"}, {"rwf", "Rwandan Franc"},
    {"sar", "Saudi Riyal"}, {"sbd", "Solomon Islands Dollar"}, {"scr", "Seychellois Rupee"},
    {"sdg", "Sudanese Pound"}, {"sek", "Swedish Krona"}, {"sgd", "Singapore Dollar"},
    {"shp", "Saint Helena Pound"}, {"sll", "Sierra Leonean Leone"}, {"sos", "Somali Shilling"},
    {"srd", "Surinamese Dollar"}, {"stn", "São Tomé and Príncipe Dobra"}, {"svc", "Salvadoran Colón"},
    {"syp", "Syrian Pound"}, {"szl", "Swazi Lilangeni"}, {"thb", "Thai Baht"},
    {"tjs", "Tajikistani Somoni"}, {"tmt", "Turkmenistani Manat"}, {"tnd", "Tunisian Dinar"},
    {"top", "Tongan Pa'anga"}, {"try", "Turkish Lira"}, {"ttd", "Trinidad and Tobago Dollar"},
    {"twd", "New Taiwan Dollar"}, {"tzs", "T Tanzanian Shilling"}, {"uah", "Ukrainian Hryvnia"},
    {"ugx", "Ugandan Shilling"}, {"usd", "US Dollar"}, {"uyu", "Uruguayan Peso"},
    {"uzs", "Uzbekistan Som"}, {"ves", "Venezuelan Bolívar"}, {"vnd", "Vietnamese Dong"},
    {"vuv", "Vanuatu Vatu"}, {"wst", "Samoan Tala"}, {"xaf", "CFA Franc BEAC"},
    {"xcd", "East Caribbean Dollar"}, {"xof", "CFA Franc BCEAO"}, {"xpf", "CFP Franc"},
    {"yer", "Yemeni Rial"}, {"zar", "South African Rand"}, {"zmw", "Zambian Kwacha"},
    {"zwl", "Zimbabwean Dollar"}
};

#endif