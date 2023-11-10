#include "esphome/core/log.h"
#include "lora_sx126x.h"

namespace esphome {
namespace lora_sx126x {

    static const char *TAG = "lora_sx126x.component";

    void LoraSX126X::setup() {

    }

    void LoraSX126X::loop() {

    }

    void LoraSX126X::dump_config(){
        ESP_LOGCONFIG(TAG, "Lora SX126X Component");
    }


    }  // namespace empty_component
}  // namespace esphome
