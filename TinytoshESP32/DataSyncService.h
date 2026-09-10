#ifndef DATA_SYNC_SERVICE_H
#define DATA_SYNC_SERVICE_H

#include "structs.h"

class DataSyncService {
public:
    bool isGlobalSyncDue(const Config& config, bool nightModeLatched) const;
    void markGlobalSynced();
    bool isDue(ScreenType screen, const Config& config, bool nightModeLatched) const;
    void markFetched(ScreenType screen);
    void runFullSync(AppState& state);
    void maybeStartBackgroundSync(AppState& state, bool nightModeLatched);

private:
    static const int NIGHT_INTERVAL_MULTIPLIER = 10;

    FetchTrackers trackers;

    bool isBackgroundSyncRunning = false;
    TaskHandle_t backgroundSyncTaskHandle = NULL;
    AppState* activeState = nullptr;
    bool activeNightModeLatched = false;

    unsigned long getGlobalIntervalMs(const Config& config, bool nightModeLatched) const;
    bool isFetchDue(unsigned long lastFetch, int customMin, const Config& config, bool nightModeLatched) const;

    static void backgroundSyncTaskTrampoline(void* parameter);
    void runBackgroundSyncBody();
};

#endif
