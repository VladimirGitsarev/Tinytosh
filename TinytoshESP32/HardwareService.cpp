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
    claimPin(config.button_pin, DEFAULT_BUTTON_PIN);
}

void HardwareService::begin(Config& config) {
    validatePins(config);

    if (config.button_type == "switch") {
        button.setup(config.button_pin, INPUT_PULLUP, true);
    } else {
        button.setup(config.button_pin, INPUT, false);
    }
    button.attachClick(onClickCb);
    button.attachDoubleClick(onDoubleClickCb);
    button.attachLongPressStart(onLongPressCb);
    button.setDebounceTicks(50);
    button.setClickTicks(150);
    button.setPressTicks(500);
    button.reset();

    Serial.printf("Hardware Configured: SDA=%d, SCL=%d, BUTTON=%d (%s)\n", config.sda_pin, config.scl_pin, config.button_pin, config.button_type.c_str());
}

void HardwareService::tick() {
    button.tick();
}
