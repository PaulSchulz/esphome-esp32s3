#include "nem_price_sensor.h"
#include "esphome/core/log.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace esphome {
    namespace nem_price_sensor {

        static const char *const TAG = "nem_price_sensor";

        void NEMPriceSensor::update() {
            HTTPClient http;
            http.begin(this->url_.c_str());
            int httpCode = http.GET();

            if (httpCode > 0) {
                String payload = http.getString();
                StaticJsonDocument<512> doc;
                DeserializationError error = deserializeJson(doc, payload);

                if (!error) {
                    if (doc.containsKey(this->json_key_)) {
                        float value = doc[this->json_key_];
                        ESP_LOGD(TAG, "Parsed value: %f", value);
                        this->publish_state(value);
                    } else {
                        ESP_LOGW(TAG, "JSON key '%s' not found", this->json_key_.c_str());
                    }
                } else {
                    ESP_LOGE(TAG, "Failed to parse JSON: %s", error.c_str());
                }
            } else {
                ESP_LOGE(TAG, "HTTP request failed with code: %d", httpCode);
            }

            http.end();
        }

    }  // namespace nem_price_sensor
}  // namespace esphome
