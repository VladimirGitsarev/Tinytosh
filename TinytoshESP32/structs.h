#ifndef STRUCTS_H
#define STRUCTS_H

#include <Arduino.h>

const int MAX_MULTI_ENTRIES = 5;

enum ScreenType {
  SCREEN_TIME,
  SCREEN_CALENDAR,
  SCREEN_WEATHER,
  SCREEN_AIR_QUALITY,
  SCREEN_DAYLIGHT,
  SCREEN_STOCK,
  SCREEN_CRYPTO,
  SCREEN_CURRENCY,
  SCREEN_PC_MONITOR,
  SCREEN_PC_MEDIA,
  SCREEN_BAMBU,
  SCREEN_FOCUS,
  NUM_SCREENS
};

inline constexpr const char* SCREEN_NAMES[] = {
  "Time & Date",
  "Calendar",
  "Weather",
  "Air Quality",
  "Daylight Info",
  "Stock Tracking",
  "Crypto Tracking",
  "Currency Exchange",
  "PC Monitor",
  "PC Media",
  "Printer Info",
  "Focus Mode"
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

enum FocusSessionState {
  FOCUS_STATE_IDLE,
  FOCUS_STATE_RUNNING,
  FOCUS_STATE_PAUSED
};

// Runtime state for the Focus Mode timer
struct FocusData {
  FocusSessionState state = FOCUS_STATE_IDLE;
  unsigned long elapsed_ms = 0;       // Time accumulated across completed running segments.
  unsigned long segment_start_ms = 0; // millis() when the current running segment began.
};

struct Holiday {
  String date;
  String name;
};

struct Config {
  // Network Data
  String device_id = "";
  String ip_address = "";
  String active_pc_id = "";

  // Hardware Setup
  int sda_pin = 8;
  int scl_pin = 9;
  int touch_pin = 10;

  // Global Settings
  bool auto_detect = true;
  float latitude = 0.0;
  float longitude = 0.0;
  String country = "";
  String country_code = "";
  String city = "";
  String timezone = ""; 
  String time_format = "24";
  bool date_display = false;
  unsigned long refresh_interval_min = 15;

  // Theme Settings
  String theme_bg = "#000000";
  String theme_card = "#111111";
  String theme_accent = "#ffffff";
  String theme_text = "#ffffff";

  // Screens Settings
  bool screen_auto_cycle = true;
  int screen_interval_sec = 15;
  int screen_order[NUM_SCREENS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

  bool show_time = true;
  bool show_calendar = true;
  bool show_weather = true;
  bool show_aqi = true;
  bool show_daylight = true;
  bool show_stock = true;
  bool show_crypto = true;
  bool show_currency = true;
  bool show_pc = true;
  bool show_media = true;
  bool show_bambu = true;

  bool hide_empty_pc = true;
  bool hide_empty_media = true;
  bool hide_empty_bambu = true;

  // Calendar Settings
  String calendar_start_day = "mon";
  bool calendar_show_holidays = true;
  bool calendar_minimal = false;

  // Weather & AQI Settings
  bool round_temps = true; 
  String temp_unit = "C";
  String aqi_type = "US";
  bool weather_hide_bar = false;
  bool aqi_hide_bar = false;

  // Daylight settings
  bool daylight_minimal = false;

  // Crypto, Currency & Stocks Settings
  String stock_symbols[MAX_MULTI_ENTRIES] = {"AAPL", "", "", "", ""};
  int stock_count = 1;

  int crypto_ids[MAX_MULTI_ENTRIES] = {90, 0, 0, 0, 0};
  int crypto_count = 1;

  String currency_bases[MAX_MULTI_ENTRIES] = {"usd", "", "", "", ""};
  String currency_targets[MAX_MULTI_ENTRIES] = {"eur", "", "", "", ""};
  int currency_multipliers[MAX_MULTI_ENTRIES] = {1, 1, 1, 1, 1};
  int currency_count = 1;

  bool crypto_fn = true;
  bool currency_fn = true;
  bool stock_fn = true;

  // Printer Settings
  String bambu_ip = "";
  String bambu_sn = "";
  String bambu_code = "";

  // Focus Mode Settings
  bool show_focus = true;
  int focus_durations[MAX_MULTI_ENTRIES] = {30, 0, 0, 0, 0};  // Minutes per custom slot (up to 5)
  int focus_duration_count = 1;                               // How many slots are actually filled in
  int focus_duration_index = 0;                               // Which slot is active
  bool focus_flash = true;                                    // Flash display when a session completes

  // Animation Settings
  uint16_t anim_mask = 62;

  // Night Mode Settings
  bool night_mode = false;
  String night_start = "23:00";
  String night_end = "06:00";
  String night_dim_start = "22:00";
  int night_action = 1; // 0: None, 1: Dim, 2: Off, 3: Dim + Off
};

struct CalendarData {
  Holiday items[30];
  int count = 0;
  int last_fetch_year = -1;
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

struct DaylightData {
  int sunrise_mins = -1;
  int sunset_mins = -1;
  int noon_mins = -1;
  int length_mins = -1;
  int last_fetch_yday = -1;
};

struct StockData {
  String symbol;
  String name;
  float price;
  float previous_close;
  float percent_change;
  bool updated = false;
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

struct PcStats {
  float cpu_percent;
  float mem_percent;
  float disk_percent;
  float net_down_kb;
  unsigned long last_update = 0;
  bool is_wifi = false;
};

struct PcMedia {
  String status;
  String name;
  String author;
  String album;
  unsigned long last_update = 0;
};

struct BambuData {
  String status = "SYNCING";
  int progress = 0;
  int time_left = 0;
  float nozzle_temp = 0.0;
  float nozzle_target = 0.0;
  float bed_temp = 0.0;
  float bed_target = 0.0;
  int layer = 0;
  int total_layers = 0;
  String file_name = "None";
  int fan_part = 0;
  int fan_aux = 0;
};

struct CountryOption {
  const char* code;
  const char* name;
};

struct StockOption {
  const char* name;
  const char* ticker;
};

struct CoinOption { 
  int id;
  const char* sym; 
};

struct CurrencyOption {
  const char* code;
  const char* name;
};

inline constexpr CountryOption allCountries[] = {
  {"AF", "Afghanistan"}, {"AL", "Albania"}, {"DZ", "Algeria"}, {"AS", "American Samoa"},
  {"AD", "Andorra"}, {"AO", "Angola"}, {"AI", "Anguilla"}, {"AQ", "Antarctica"},
  {"AG", "Antigua and Barbuda"}, {"AR", "Argentina"}, {"AM", "Armenia"}, {"AW", "Aruba"},
  {"AU", "Australia"}, {"AT", "Austria"}, {"AZ", "Azerbaijan"}, {"BS", "Bahamas"},
  {"BH", "Bahrain"}, {"BD", "Bangladesh"}, {"BB", "Barbados"}, {"BY", "Belarus"},
  {"BE", "Belgium"}, {"BZ", "Belize"}, {"BJ", "Benin"}, {"BM", "Bermuda"},
  {"BT", "Bhutan"}, {"BO", "Bolivia"}, {"BA", "Bosnia and Herzegovina"}, {"BW", "Botswana"},
  {"BR", "Brazil"}, {"IO", "British Indian Ocean Territory"}, {"VG", "British Virgin Islands"},
  {"BN", "Brunei"}, {"BG", "Bulgaria"}, {"BF", "Burkina Faso"}, {"BI", "Burundi"},
  {"CV", "Cabo Verde"}, {"KH", "Cambodia"}, {"CM", "Cameroon"}, {"CA", "Canada"},
  {"KY", "Cayman Islands"}, {"CF", "Central African Republic"}, {"TD", "Chad"},
  {"CL", "Chile"}, {"CN", "China"}, {"CX", "Christmas Island"}, {"CC", "Cocos Islands"},
  {"CO", "Colombia"}, {"KM", "Comoros"}, {"CD", "Congo (DRC)"}, {"CG", "Congo (Republic)"},
  {"CK", "Cook Islands"}, {"CR", "Costa Rica"}, {"CI", "Cote d'Ivoire"}, {"HR", "Croatia"},
  {"CU", "Cuba"}, {"CW", "Curacao"}, {"CY", "Cyprus"}, {"CZ", "Czechia"},
  {"DK", "Denmark"}, {"DJ", "Djibouti"}, {"DM", "Dominica"}, {"DO", "Dominican Republic"},
  {"EC", "Ecuador"}, {"EG", "Egypt"}, {"SV", "El Salvador"}, {"GQ", "Equatorial Guinea"},
  {"ER", "Eritrea"}, {"EE", "Estonia"}, {"SZ", "Eswatini"}, {"ET", "Ethiopia"},
  {"FK", "Falkland Islands"}, {"FO", "Faroe Islands"}, {"FJ", "Fiji"}, {"FI", "Finland"},
  {"FR", "France"}, {"GF", "French Guiana"}, {"PF", "French Polynesia"}, {"GA", "Gabon"},
  {"GM", "Gambia"}, {"GE", "Georgia"}, {"DE", "Germany"}, {"GH", "Ghana"},
  {"GI", "Gibraltar"}, {"GR", "Greece"}, {"GL", "Greenland"}, {"GD", "Grenada"},
  {"GP", "Guadeloupe"}, {"GU", "Guam"}, {"GT", "Guatemala"}, {"GG", "Guernsey"},
  {"GN", "Guinea"}, {"GW", "Guinea-Bissau"}, {"GY", "Guyana"}, {"HT", "Haiti"},
  {"HN", "Honduras"}, {"HK", "Hong Kong"}, {"HU", "Hungary"}, {"IS", "Iceland"},
  {"IN", "India"}, {"ID", "Indonesia"}, {"IR", "Iran"}, {"IQ", "Iraq"},
  {"IE", "Ireland"}, {"IM", "Isle of Man"}, {"IL", "Israel"}, {"IT", "Italy"},
  {"JM", "Jamaica"}, {"JP", "Japan"}, {"JE", "Jersey"}, {"JO", "Jordan"},
  {"KZ", "Kazakhstan"}, {"KE", "Kenya"}, {"KI", "Kiribati"}, {"KW", "Kuwait"},
  {"KG", "Kyrgyzstan"}, {"LA", "Laos"}, {"LV", "Latvia"}, {"LB", "Lebanon"},
  {"LS", "Lesotho"}, {"LR", "Liberia"}, {"LY", "Libya"}, {"LI", "Liechtenstein"},
  {"LT", "Lithuania"}, {"LU", "Luxembourg"}, {"MO", "Macao"}, {"MG", "Madagascar"},
  {"MW", "Malawi"}, {"MY", "Malaysia"}, {"MV", "Maldives"}, {"ML", "Mali"},
  {"MT", "Malta"}, {"MH", "Marshall Islands"}, {"MQ", "Martinique"}, {"MR", "Mauritania"},
  {"MU", "Mauritius"}, {"YT", "Mayotte"}, {"MX", "Mexico"}, {"FM", "Micronesia"},
  {"MD", "Moldova"}, {"MC", "Monaco"}, {"MN", "Mongolia"}, {"ME", "Montenegro"},
  {"MS", "Montserrat"}, {"MA", "Morocco"}, {"MZ", "Mozambique"}, {"MM", "Myanmar"},
  {"NA", "Namibia"}, {"NR", "Nauru"}, {"NP", "Nepal"}, {"NL", "Netherlands"},
  {"NC", "New Caledonia"}, {"NZ", "New Zealand"}, {"NI", "Nicaragua"}, {"NE", "Niger"},
  {"NG", "Nigeria"}, {"NU", "Niue"}, {"NF", "Norfolk Island"}, {"KP", "North Korea"},
  {"MK", "North Macedonia"}, {"MP", "Northern Mariana Islands"}, {"NO", "Norway"},
  {"OM", "Oman"}, {"PK", "Pakistan"}, {"PW", "Palau"}, {"PS", "Palestine"},
  {"PA", "Panama"}, {"PG", "Papua New Guinea"}, {"PY", "Paraguay"}, {"PE", "Peru"},
  {"PH", "Philippines"}, {"PN", "Pitcairn"}, {"PL", "Poland"}, {"PT", "Portugal"},
  {"PR", "Puerto Rico"}, {"QA", "Qatar"}, {"RE", "Reunion"}, {"RO", "Romania"},
  {"RU", "Russia"}, {"RW", "Rwanda"}, {"WS", "Samoa"}, {"SM", "San Marino"},
  {"ST", "Sao Tome and Principe"}, {"SA", "Saudi Arabia"}, {"SN", "Senegal"},
  {"RS", "Serbia"}, {"SC", "Seychelles"}, {"SL", "Sierra Leone"}, {"SG", "Singapore"},
  {"SX", "Sint Maarten"}, {"SK", "Slovakia"}, {"SI", "Slovenia"}, {"SB", "Solomon Islands"},
  {"SO", "Somalia"}, {"ZA", "South Africa"}, {"GS", "South Georgia"}, {"KR", "South Korea"},
  {"SS", "South Sudan"}, {"ES", "Spain"}, {"LK", "Sri Lanka"}, {"BL", "St. Barthelemy"},
  {"KN", "St. Kitts and Nevis"}, {"LC", "St. Lucia"}, {"MF", "St. Martin"},
  {"PM", "St. Pierre and Miquelon"}, {"VC", "St. Vincent and Grenadines"}, {"SD", "Sudan"},
  {"SR", "Suriname"}, {"SJ", "Svalbard and Jan Mayen"}, {"SE", "Sweden"},
  {"CH", "Switzerland"}, {"SY", "Syria"}, {"TW", "Taiwan"}, {"TJ", "Tajikistan"},
  {"TZ", "Tanzania"}, {"TH", "Thailand"}, {"TL", "Timor-Leste"}, {"TG", "Togo"},
  {"TK", "Tokelau"}, {"TO", "Tonga"}, {"TT", "Trinidad and Tobago"}, {"TN", "Tunisia"},
  {"TR", "Turkey"}, {"TM", "Turkmenistan"}, {"TC", "Turks and Caicos Islands"},
  {"TV", "Tuvalu"}, {"VI", "U.S. Virgin Islands"}, {"UG", "Uganda"}, {"UA", "Ukraine"},
  {"AE", "United Arab Emirates"}, {"GB", "United Kingdom"}, {"US", "United States"},
  {"UY", "Uruguay"}, {"UZ", "Uzbekistan"}, {"VU", "Vanuatu"}, {"VA", "Vatican City"},
  {"VE", "Venezuela"}, {"VN", "Vietnam"}, {"WF", "Wallis and Futuna"},
  {"EH", "Western Sahara"}, {"YE", "Yemen"}, {"ZM", "Zambia"}, {"ZW", "Zimbabwe"}
};

inline constexpr StockOption topStocks[] = {
  // Broad Market & Sector ETFs
  {"S&P 500 ETF", "SPY"}, {"Invesco QQQ (Tech)", "QQQ"}, {"Dow Jones ETF", "DIA"},
  {"Vanguard Total Stock", "VTI"}, {"Vanguard S&P 500", "VOO"}, 
  {"Semiconductor ETF", "SMH"}, {"Financial Select", "XLF"}, 
  {"Health Care Select", "XLV"}, {"Energy Select", "XLE"},

  // Mega-Cap Tech & Semiconductors
  {"Apple Inc.", "AAPL"}, {"Microsoft Corp.", "MSFT"}, {"NVIDIA Corp.", "NVDA"},
  {"Alphabet Inc.", "GOOG"}, {"Amazon.com Inc.", "AMZN"}, {"Meta Platforms", "META"},
  {"Tesla Inc.", "TSLA"}, {"Taiwan Semiconductor", "TSM"}, {"Broadcom Inc.", "AVGO"},
  {"ASML Holding", "ASML"}, {"Intel Corp.", "INTC"}, {"Qualcomm Inc.", "QCOM"},
  {"Texas Instruments", "TXN"}, {"Micron Technology", "MU"}, {"ARM Holdings", "ARM"},

  // Software, Cloud & Cybersecurity
  {"Salesforce Inc.", "CRM"}, {"Adobe Inc.", "ADBE"}, {"ServiceNow", "NOW"},
  {"Snowflake Inc.", "SNOW"}, {"CrowdStrike", "CRWD"}, {"Palo Alto Networks", "PANW"},
  {"Fortinet", "FTNT"}, {"Palantir Tech", "PLTR"}, {"Datadog Inc.", "DDOG"},

  // Finance, FinTech & Crypto Proxies
  {"JPMorgan Chase", "JPM"}, {"Visa Inc.", "V"}, {"Mastercard Inc.", "MA"},
  {"Bank of America", "BAC"}, {"Berkshire Hathaway", "BRK.B"}, {"Wells Fargo", "WFC"},
  {"Goldman Sachs", "GS"}, {"Morgan Stanley", "MS"}, {"American Express", "AXP"},
  {"PayPal Holdings", "PYPL"}, {"Block Inc. (Square)", "SQ"}, {"Coinbase Global", "COIN"},
  {"MicroStrategy", "MSTR"},

  // Retail, Food & Consumer Discretionary
  {"Walmart Inc.", "WMT"}, {"Costco Wholesale", "COST"}, {"The Home Depot", "HD"},
  {"Lowe's Companies", "LOW"}, {"Target Corp.", "TGT"}, {"McDonald's Corp.", "MCD"},
  {"Starbucks Corp.", "SBUX"}, {"Nike Inc.", "NKE"}, {"Lululemon", "LULU"},
  {"Procter & Gamble", "PG"}, {"The Coca-Cola Co.", "KO"}, {"PepsiCo Inc.", "PEP"},

  // Healthcare, Pharma & Biotech
  {"Eli Lilly and Co.", "LLY"}, {"UnitedHealth Group", "UNH"}, {"Johnson & Johnson", "JNJ"},
  {"AbbVie Inc.", "ABBV"}, {"Merck & Co.", "MRK"}, {"Pfizer Inc.", "PFE"},
  {"Novo Nordisk (ADR)", "NVO"}, {"Thermo Fisher", "TMO"}, {"Intuitive Surgical", "ISRG"},

  // Energy, Industrials & Defense
  {"Exxon Mobil", "XOM"}, {"Chevron Corp.", "CVX"}, {"Caterpillar Inc.", "CAT"},
  {"General Electric", "GE"}, {"Honeywell Intl", "HON"}, {"The Boeing Company", "BA"},
  {"Union Pacific", "UNP"}, {"Lockheed Martin", "LMT"}, {"RTX Corporation", "RTX"},

  // Media, Entertainment & Telecom
  {"The Walt Disney Co.", "DIS"}, {"Netflix Inc.", "NFLX"}, {"Comcast Corp.", "CMCSA"},
  {"Spotify Technology", "SPOT"}, {"AT&T Inc.", "T"}, {"Verizon Comm.", "VZ"},
  {"T-Mobile US", "TMUS"},

  // International ADRs & E-commerce
  {"Alibaba Group", "BABA"}, {"Sony Group Corp.", "SONY"}, {"Shopify Inc.", "SHOP"},
  {"MercadoLibre", "MELI"}, {"Toyota Motor Corp.", "TM"}, {"Ferrari N.V.", "RACE"},

  // Transport & Travel
  {"Uber Technologies", "UBER"}, {"Airbnb Inc.", "ABNB"}
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

struct AppState {
  Config config;
  CalendarData calendar;
  WeatherData weather;
  AirQualityData aqi;
  DaylightData daylight;
  CryptoData cryptos[MAX_MULTI_ENTRIES];
  CurrencyData currencies[MAX_MULTI_ENTRIES];
  StockData stocks[MAX_MULTI_ENTRIES];
  PcStats pc;
  PcMedia media;
  BambuData bambu;
  FocusData focus;
};
#endif