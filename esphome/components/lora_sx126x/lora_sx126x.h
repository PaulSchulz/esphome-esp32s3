#pragma once

#include "esphome/core/component.h"
// #include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace lora_sx126x {

class LoraSX126X : public Component {
 public:
    void setup() override;
    void loop() override;
    void dump_config() override;
};


}  // namespace empty_component
}  // namespace esphome
