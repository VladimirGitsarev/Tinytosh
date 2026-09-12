#include "HardwareService.h"
#include <Arduino.h>

HardwareService::HardwareService(ButtonCallback onClick, ButtonCallback onDoubleClick, ButtonCallback onLongPress)
    : onClickCb(onClick), onDoubleClickCb(onDoubleClick), onLongPressCb(onLongPress) {}

void HardwareService::validatePins(Config& config) {
    bool used_pins[MAX_GPIO_PIN + 1] = {false};

    auto claimPin = [&](int &pin, int default_pin) {

        if (pin < MIN_GPIO_PIN || pin > MAX_GPIO_PIN) {
            pin = default_pin;
        }

        if (used_pins[pin]) {
            pin = default_pin;

            if (used_pins[pin]) {
                for (int i = MIN_GPIO_PIN; i <= MAX_GPIO_PIN; i++) {
                    if (!used_pins[i]) {
                        pin = i;
                        break;
                    }
                }
            }
        }

        used_pins[pin] = true;
    };

    claimPin(config.sda_pin, DEFAULT_SDA_PIN);
    claimPin(config.scl_pin, DEFAULT_SCL_PIN);
    claimPin(config.touch_pin, DEFAULT_TOUCH_PIN);
}

void HardwareService::begin(Config& config) {
    validatePins(config);

    button.setup(config.touch_pin, INPUT, false);
    button.attachClick(onClickCb);
    button.attachDoubleClick(onDoubleClickCb);
    button.attachLongPressStart(onLongPressCb);
    button.setDebounceTicks(50);
    button.setClickTicks(150);
    button.setPressTicks(500);
    button.reset();

    Serial.printf("Hardware Configured: SDA=%d, SCL=%d, TOUCH=%d\n", config.sda_pin, config.scl_pin, config.touch_pin);
}

void HardwareService::tick() {
    button.tick();
}
