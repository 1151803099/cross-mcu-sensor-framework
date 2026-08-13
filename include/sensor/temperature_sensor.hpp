#pragma once

#include "platform/temperature_driver.hpp"

#include <memory>
#include <utility>

namespace sensor_framework::sensor {

class ISensor {
public:
    virtual ~ISensor() = default;
    virtual bool initialize() = 0;
    virtual bool read(float& value) = 0;
    virtual const char* type() const = 0;
};

class TemperatureSensor final : public ISensor {
public:
    explicit TemperatureSensor(std::unique_ptr<platform::ITemperatureDriver> driver)
        : driver_(std::move(driver)) {}

    bool initialize() override { return driver_ != nullptr && driver_->initialize(); }

    bool read(float& value) override {
        std::int32_t raw_value = 0;
        if (driver_ == nullptr || !driver_->read_raw(raw_value)) {
            return false;
        }
        value = driver_->raw_to_celsius(raw_value);
        return true;
    }

    const char* type() const override { return "temperature"; }

private:
    std::unique_ptr<platform::ITemperatureDriver> driver_;
};

}  // namespace sensor_framework::sensor
