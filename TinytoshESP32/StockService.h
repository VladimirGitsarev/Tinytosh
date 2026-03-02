#ifndef STOCK_SERVICE_H
#define STOCK_SERVICE_H

#include "structs.h"


class StockService {
public:
    bool fetchStock(const String& symbol, StockData &data);

private:
    const char* STOCK_API_URL = "https://stooq.com/q/l/";
};

#endif