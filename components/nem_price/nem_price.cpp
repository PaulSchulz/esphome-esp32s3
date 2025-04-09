#include "nem_price.h"
#include "esphome/core/log.h"
#include "Arduino.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace esphome {
    namespace nem_price {

        static const char *const TAG = "nem_price";

        void NEMPrice::update() {
            ESP_LOGD(TAG, "NEMPrice::update()");

            HTTPClient http;
            http.begin(this->url_.c_str());
            int httpCode = http.GET();

            if (httpCode > 0) {
                String payload = http.getString();
                // StaticJsonDocument<512> doc;
                DynamicJsonDocument doc(14336); // (* 14 1024) ;; 14336
                DeserializationError error = deserializeJson(doc, payload);
                if (error) {
                    ESP_LOGE(TAG, "JSON parsing failed: %s", error.c_str());
                    http.end();
                    return;
                }

                float price = 0.0;

                JsonArray summary = doc["ELEC_NEM_SUMMARY"].as<JsonArray>();
                bool found = false;
                for (JsonObject entry : summary) {
                    const char* region = entry["REGIONID"];
                    if (region != nullptr && strcmp(region, this->region_.c_str()) == 0) {
                        price = entry["PRICE"];
                        price = price / 1000.0; // $ per kWh
                        ESP_LOGD("nem_price_sensor",
                                 "Extracted Price for %s: %f",
                                 this->region_.c_str(),
                                 price);
                        found = true;
                        break;
                    }
                }
                if(found){
                    this->publish_state(price);
                }
                if (!found) {
                    ESP_LOGW("nem_price_sensor", "No entry with REGIONID '%s' found.",
                             this->region_.c_str()
                             );
                }

            } else {
                ESP_LOGE(TAG, "HTTP request failed with code: %d", httpCode);
            }

            http.end();
        }


    }  // namespace nem_price_sensor
}  // namespace esphome
