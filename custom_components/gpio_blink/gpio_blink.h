#pragma once

#include "esphome/core/component.h"

namespace esphome {
    namespace gpio_blink {
        class GPIOBlink : public Component {

        public:
            void setup() override;
            void loop() override;
            void dump_config() override;
        };
    }  // namespace gpio_blink
}  // namespace esphome
