#include "nem_price_sensor.h"
#include "esphome/core/log.h"
#include "Arduino.h"
#include <WiFi.h>            // Include the WiFi library first
#include <WiFiClientSecure.h> // This provides WiFiClientSecure.h for HTTPS
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

                    // Hardcoded path for now, or later make this configurable
                    JsonVariant variant = doc["nem"]["REGIONS"]["SA1"]["PRICE"];

                    if (!variant.isNull()) {
                        float value = variant.as<float>();
                        ESP_LOGD(TAG, "Parsed NEM SA1 Price: %f", value);
                        this->publish_state(value);
                    } else {
                        ESP_LOGW(TAG, "Price key not found in JSON.");
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
