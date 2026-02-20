#ifndef CURRENCY_SERVICE_H
#define CURRENCY_SERVICE_H

#include "structs.h"

class CurrencyService {
public:
    bool fetchRate(const String& base, const String& target, CurrencyData &data);

private:
    const char* CURRENCY_API_URLS[2] = {
        "https://cdn.jsdelivr.net/npm/@fawazahmed0/currency-api@latest/v1/currencies/",
        "https://latest.currency-api.pages.dev/v1/currencies/"
    };
};

#endif