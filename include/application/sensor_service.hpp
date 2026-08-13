#pragma once

#include "sensor/filter.hpp"
#include "sensor/temperature_sensor.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace sensor_framework::application {

struct SensorReading {
    const char* sensor_type;
    float value_celsius;
    std::uint32_t timestamp_ms;
};

class IReadingObserver {
public:
    virtual ~IReadingObserver() = default;
    virtual void on_reading(const SensorReading& reading) = 0;
};

class SensorService {
public:
    SensorService(std::unique_ptr<sensor::ISensor> sensor, std::unique_ptr<sensor::IFilter> filter)
        : sensor_(std::move(sensor)), filter_(std::move(filter)) {}

    bool initialize() { return sensor_ != nullptr && filter_ != nullptr && sensor_->initialize(); }

    void set_filter(std::unique_ptr<sensor::IFilter> filter) {
        if (filter != nullptr) {
            filter_ = std::move(filter);
        }
    }

    const char* filter_name() const { return filter_ == nullptr ? "unavailable" : filter_->name(); }

    void subscribe(IReadingObserver& observer) { observers_.push_back(&observer); }

    void unsubscribe(IReadingObserver& observer) {
        observers_.erase(std::remove(observers_.begin(), observers_.end(), &observer), observers_.end());
    }

    bool sample_and_publish(std::uint32_t timestamp_ms) {
        float raw_celsius = 0.0F;
        if (sensor_ == nullptr || filter_ == nullptr || !sensor_->read(raw_celsius)) {
            return false;
        }

        const SensorReading reading {sensor_->type(), filter_->process(raw_celsius), timestamp_ms};
        for (IReadingObserver* observer : observers_) {
            observer->on_reading(reading);
        }
        return true;
    }

private:
    std::unique_ptr<sensor::ISensor> sensor_;
    std::unique_ptr<sensor::IFilter> filter_;
    std::vector<IReadingObserver*> observers_;
};

}  // namespace sensor_framework::application
