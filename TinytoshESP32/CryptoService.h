#ifndef CRYPTO_SERVICE_H
#define CRYPTO_SERVICE_H

#include "structs.h"

class CryptoService {
public:
    bool fetchPrice(int id, CryptoData &data);

private:
    const char* CRYPTO_API_URL = "https://api.coinlore.net/api/ticker/";
};

#endif