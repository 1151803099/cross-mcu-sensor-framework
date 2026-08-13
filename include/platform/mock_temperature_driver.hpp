#pragma once

#include "platform/temperature_driver.hpp"

#include <cstddef>
#include <utility>
#include <vector>

namespace sensor_framework::platform {

class MockTemperatureDriver final : public ITemperatureDriver {
public:
    explicit MockTemperatureDriver(std::vector<std::int32_t> samples) : samples_(std::move(samples)) {}

    bool initialize() override { return !samples_.empty(); }

    bool read_raw(std::int32_t& raw_value) override {
        if (samples_.empty()) {
            return false;
        }
        raw_value = samples_[next_sample_++ % samples_.size()];
        return true;
    }

    float raw_to_celsius(std::int32_t raw_value) const override {
        return static_cast<float>(raw_value) / 10.0F;
    }

private:
    std::vector<std::int32_t> samples_;
    std::size_t next_sample_ {0U};
};

}  // namespace sensor_framework::platform
