#include "FocusService.h"

FocusService::FocusService() {}

bool FocusService::onDoublePress(int currentScreen, Config& config, const FocusData& focus) {
    if (currentScreen != SCREEN_FOCUS) return false;
    if (focus.state != FOCUS_STATE_IDLE) return false;

    cycleDuration(config);
    return true;
}

bool FocusService::onTriplePress(int currentScreen, FocusData& focus) {
    if (currentScreen != SCREEN_FOCUS) return false;

    toggleStartPause(focus);
    return true;
}

bool FocusService::allowScreenSwitch(int currentScreen, FocusData& focus) {
    if (currentScreen != SCREEN_FOCUS) return true;
    if (focus.state == FOCUS_STATE_RUNNING) return false;
    if (focus.state == FOCUS_STATE_PAUSED) return true;
    return true;
}

bool FocusService::tick(FocusData& focus, const Config& config) {
    if (focus.state != FOCUS_STATE_RUNNING) return false;
    if (getRemainingMs(focus, config) > 0) return false;

    // Countdown reached zero: session complete.
    focus.state = FOCUS_STATE_IDLE;
    focus.elapsed_ms = 0;
    focus.segment_start_ms = 0;
    return true;
}

void FocusService::toggleStartPause(FocusData& focus) {
    unsigned long now = millis();

    switch (focus.state) {
        case FOCUS_STATE_IDLE:
            focus.elapsed_ms = 0;
            focus.segment_start_ms = now;
            focus.state = FOCUS_STATE_RUNNING;
            break;

        case FOCUS_STATE_RUNNING:
            focus.elapsed_ms += now - focus.segment_start_ms;
            focus.state = FOCUS_STATE_PAUSED;
            break;

        case FOCUS_STATE_PAUSED:
            focus.segment_start_ms = now;
            focus.state = FOCUS_STATE_RUNNING;
            break;
    }
}

void FocusService::cycleDuration(Config& config) {
    if (config.focus_duration_count <= 0) return;
    config.focus_duration_index = (config.focus_duration_index + 1) % config.focus_duration_count;
}

void FocusService::reset(FocusData& focus) {
    focus.state = FOCUS_STATE_IDLE;
    focus.elapsed_ms = 0;
    focus.segment_start_ms = 0;
}

bool FocusService::isLocked(const FocusData& focus) {
    return focus.state == FOCUS_STATE_RUNNING;
}

unsigned long FocusService::getElapsedMs(const FocusData& focus) {
    if (focus.state == FOCUS_STATE_RUNNING) {
        return focus.elapsed_ms + (millis() - focus.segment_start_ms);
    }
    return focus.elapsed_ms;
}

unsigned long FocusService::getRemainingMs(const FocusData& focus, const Config& config) {
    unsigned long totalMs = (unsigned long)getActiveDurationMinutes(config) * 60000UL;
    unsigned long elapsed = getElapsedMs(focus);
    if (elapsed >= totalMs) return 0;
    return totalMs - elapsed;
}

int FocusService::getActiveDurationMinutes(const Config& config) {
    if (config.focus_duration_count <= 0) return 30; // Nothing configured: safe fallback.

    int idx = config.focus_duration_index;
    if (idx < 0) idx = 0;
    if (idx >= config.focus_duration_count) idx = config.focus_duration_count - 1;
    if (idx >= MAX_MULTI_ENTRIES) idx = MAX_MULTI_ENTRIES - 1;

    int minutes = config.focus_durations[idx];
    return (minutes > 0) ? minutes : 30;
}