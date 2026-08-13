#pragma once

#include <cstdint>

namespace sensor_framework::platform {

// This is the only interface that must be implemented for a new MCU/driver.
class ITemperatureDriver {
public:
    virtual ~ITemperatureDriver() = default;

    virtual bool initialize() = 0;
    virtual bool read_raw(std::int32_t& raw_value) = 0;
    virtual float raw_to_celsius(std::int32_t raw_value) const = 0;
};

}  // namespace sensor_framework::platform

