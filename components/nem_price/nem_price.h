// -*- c++ -*-
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
    namespace nem_price {

        class NEMPrice : public PollingComponent, public sensor::Sensor {

        public:
            void set_url(const std::string &url) { url_ = url; }
            void set_region(const std::string &region) { this->region_ = region; }

            void setup() override {}
            void update() override;

        protected:
            std::string url_;
            std::string json_key_;
            std::string region_{"NSW1"};  // default region if not set in YAML
        };

    }  // namespace nem_price
}  // namespace esphome
