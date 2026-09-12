#ifndef HARDWARE_SERVICE_H
#define HARDWARE_SERVICE_H

#include <OneButton.h>
#include "structs.h"

typedef void (*ButtonCallback)(void);

class HardwareService {
public:
    HardwareService(ButtonCallback onClick, ButtonCallback onDoubleClick, ButtonCallback onLongPress);

    void begin(Config& config);
    void tick();

private:
    static const int MIN_GPIO_PIN = 0;
    static const int MAX_GPIO_PIN = 21;
    static const int DEFAULT_SDA_PIN = 8;
    static const int DEFAULT_SCL_PIN = 9;
    static const int DEFAULT_TOUCH_PIN = 10;

    OneButton button;
    ButtonCallback onClickCb;
    ButtonCallback onDoubleClickCb;
    ButtonCallback onLongPressCb;

    void validatePins(Config& config);
};

#endif
