#pragma once

#include "application/sensor_service.hpp"

#include <iostream>

namespace sensor_framework::application {

class GuiObserver final : public IReadingObserver {
public:
    void on_reading(const SensorReading& reading) override {
        std::cout << "[GUI] " << reading.sensor_type << " = " << reading.value_celsius << " C\n";
    }
};

// Transport is deliberately abstracted: this class owns payload mapping only,
// while a production adapter can send it through an MQTT SDK or a module UART.
class MqttObserver final : public IReadingObserver {
public:
    void on_reading(const SensorReading& reading) override {
        std::cout << "[MQTT adapter] topic=device/sensors/" << reading.sensor_type
                  << ", payload={\"value_celsius\":" << reading.value_celsius
                  << ",\"timestamp_ms\":" << reading.timestamp_ms << "}\n";
    }
};

class BusinessObserver final : public IReadingObserver {
public:
    explicit BusinessObserver(float high_temperature_celsius) : high_temperature_celsius_(high_temperature_celsius) {}

    void on_reading(const SensorReading& reading) override {
        if (reading.value_celsius >= high_temperature_celsius_) {
            std::cout << "[Business] high-temperature event: " << reading.value_celsius << " C\n";
        }
    }

private:
    float high_temperature_celsius_;
};

}  // namespace sensor_framework::application

