#include "NightModeService.h"

#include <Arduino.h>

void NightModeService::recordInteraction() {
    lastInteractionTime = millis();
    lastScreenUpdate = 0;
}

bool NightModeService::isLatched() const {
    return nightModeLatched;
}

bool NightModeService::isScreenOffAction(int activeAction) const {
    return nightModeLatched && activeAction == 2;
}

bool NightModeService::wasScreenOff(int activeAction) const {
    return isScreenOffAction(activeAction) && (millis() - lastInteractionTime >= NIGHT_WAKE_DURATION_MS);
}

bool NightModeService::isTemporarilyAwake(int activeAction) const {
    return isScreenOffAction(activeAction) && (millis() - lastInteractionTime < NIGHT_WAKE_DURATION_MS);
}

bool NightModeService::update(int activeAction, bool isOnFirstEnabledScreen) {
    bool nightScheduleActive = (activeAction != -1);
    bool justExited = false;

    if (!nightScheduleActive) {
        if (nightModeLatched) {
            Serial.println("☀️ Morning reached: Exiting Night Mode and resuming normal operation.");
            nightModeLatched = false;
            lastScreenUpdate = 0;
            justExited = true;
        }
    } else if (!nightModeLatched) {
        if (isOnFirstEnabledScreen) {
            Serial.println("🌙 Night Mode Latched: Reached the primary screen. Applying night settings.");
            nightModeLatched = true;
            lastScreenUpdate = 0;
            lastInteractionTime = millis() - NIGHT_WAKE_DURATION_MS;
        }
    }

    return justExited;
}

void NightModeService::reset() {
    nightModeLatched = false;
}

unsigned long NightModeService::getRefreshIntervalMs(int activeAction) const {
    if (!nightModeLatched) return NORMAL_REFRESH_MS;
    return (activeAction == 2) ? NIGHT_DISPLAY_OFF_REFRESH_MS : NIGHT_DIM_REFRESH_MS;
}

bool NightModeService::isRedrawDue(unsigned long refreshIntervalMs) const {
    return (millis() - lastScreenUpdate >= refreshIntervalMs) || lastScreenUpdate == 0;
}

void NightModeService::markRedrawn() {
    lastScreenUpdate = millis();
}
