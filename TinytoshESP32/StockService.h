#ifndef STOCK_SERVICE_H
#define STOCK_SERVICE_H

#include "structs.h"

class StockService {
public:
    bool fetchStock(const String& symbol, const String& apiKey, StockData &data);

private:
    static constexpr const char* STOCK_API_URL = "https://finnhub.io/api/v1/quote";
};

#endif