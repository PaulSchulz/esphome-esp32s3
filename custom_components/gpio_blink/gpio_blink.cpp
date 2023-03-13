#include "esphome/core/log.h"
#include "gpio_blink.h"

#include <Arduino.h>

#include <FastLED.h>
#define NUM_LEDS 10
CRGB leds[NUM_LEDS];
// void setup() { FastLED.addLeds<NEOPIXEL, 6>(leds, NUM_LEDS); }
// void loop() {
//     leds[0] = CRGB::White; FastLED.show(); delay(30);
//     leds[0] = CRGB::Black; FastLED.show(); delay(30);
// }

// #define PIN_LED 35
#define PIN_LED 7

namespace esphome {
    namespace gpio_blink {

        static const char *TAG = "gpio_blink";

        void GPIOBlink::setup() {
            ESP_LOGD(TAG, "Setup: PIN_LED=%d", PIN_LED);
            pinMode(PIN_LED, OUTPUT);

            FastLED.addLeds<NEOPIXEL, 6>(leds, NUM_LEDS);
        }

        unsigned long previousMillis = 0UL;
        unsigned long interval = 10000UL;
        unsigned long interval_flash = 1000UL;
        unsigned int  led_status = LOW;
        unsigned int  led_status_prev = LOW;

        void GPIOBlink::loop() {

            unsigned long currentMillis = millis();
            if(currentMillis - previousMillis > interval)
            {
                /* The Arduino executes this code once every second
                 *  (interval = 1000 (ms) = 1 second).
                 */

                // Don't forget to update the previousMillis value
                previousMillis = currentMillis;
                ESP_LOGD(TAG, "Tick LED (on)");

                led_status = HIGH;
            }

            if(led_status == HIGH
               && currentMillis - previousMillis > interval_flash)
            {
                led_status = LOW;
                ESP_LOGD(TAG, "     LED (off)");
            }

            if(led_status != led_status_prev) {
                digitalWrite(PIN_LED, led_status);
                led_status_prev = led_status;

                if(led_status == HIGH){
                    leds[0] = 0xFF0000;
                    FastLED.show();
                }
                if(led_status == LOW){
                    leds[0] = 0x0000FF;
                    FastLED.show();
                }
            }
        }

        void GPIOBlink::dump_config(){
            ESP_LOGCONFIG(TAG, "GPIO Blink");
        }

    }  // namespace empty_component
}  // namespace esphome
