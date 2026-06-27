#ifndef FOCUS_SERVICE_H
#define FOCUS_SERVICE_H

#include <Arduino.h>
#include "structs.h"

class FocusService {
public:
    FocusService();

    bool onDoublePress(int currentScreen, Config& config, const FocusData& focus);

    bool onTriplePress(int currentScreen, FocusData& focus);

    bool allowScreenSwitch(int currentScreen, FocusData& focus);

    bool tick(FocusData& focus, const Config& config);

    void toggleStartPause(FocusData& focus);

    void cycleDuration(Config& config);

    void reset(FocusData& focus);

    bool isLocked(const FocusData& focus);

    unsigned long getElapsedMs(const FocusData& focus);

    unsigned long getRemainingMs(const FocusData& focus, const Config& config);

    static int getActiveDurationMinutes(const Config& config);
};

#endif