#ifndef NIGHT_MODE_SERVICE_H
#define NIGHT_MODE_SERVICE_H


class NightModeService {
public:
    void recordInteraction();

    bool isLatched() const;
    bool wasScreenOff(int activeAction) const;
    bool isTemporarilyAwake(int activeAction) const;
    bool isScreenOffAction(int activeAction) const;
    bool update(int activeAction, bool isOnFirstEnabledScreen);
    void reset();
    unsigned long getRefreshIntervalMs(int activeAction) const;
    bool isRedrawDue(unsigned long refreshIntervalMs) const;
    void markRedrawn();

private:
    bool nightModeLatched = false;
    unsigned long lastInteractionTime = 0;
    unsigned long lastScreenUpdate = 0;

    static const unsigned long NORMAL_REFRESH_MS = 1000;
    static const unsigned long NIGHT_DIM_REFRESH_MS = 10000;
    static const unsigned long NIGHT_DISPLAY_OFF_REFRESH_MS = 60000;
    static const unsigned long NIGHT_WAKE_DURATION_MS = 30000;
};

#endif
