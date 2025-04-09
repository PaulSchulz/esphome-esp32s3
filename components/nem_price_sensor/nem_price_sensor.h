// -*- c++ -*-
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
    namespace nem_price_sensor {

        class NEMPriceSensor : public PollingComponent, public sensor::Sensor {

        public:
            void set_url(const std::string &url) { url_ = url; }

            void set_json_key(const std::string &key) { json_key_ = key; }

            void setup() override {}
            void update() override;

        protected:
            std::string url_;
            std::string json_key_;
        };

    }  // namespace nem_price_sensor
}  // namespace esphome
