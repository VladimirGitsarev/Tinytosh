#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Preferences.h>

#include "structs.h"

class ConfigManager {
public:
    ConfigManager(const char* ns);
    void loadConfig(Config& config);
    void saveConfig(const Config& config);
    void clearAllPreferences();

private:
    const char* PREF_NAMESPACE;
    Preferences preferences;
};

#endif