// My Custome Component
// See: https://esphome.io/custom/custom_component.html
#include "esphome.h"

class MyCustomComponent : public Component {
public:
    unsigned long previousMillis = 0UL;
    unsigned long interval = 10000UL;

    void setup() override {
        // You can also log messages
        ESP_LOGD("custom", "Setup Custom Timer (10s interval)");
    }

    void loop() override {
        // This will be called very often after setup time.
        // think of it as the loop() call in Arduino

        unsigned long currentMillis = millis();

        if(currentMillis - previousMillis > interval)
        {
            /* The Arduino executes this code once every second
             *  (interval = 1000 (ms) = 1 second).
             */

            // Don't forget to update the previousMillis value
            previousMillis = currentMillis;
            ESP_LOGD("custom", "Tick");
        }
    }
};
