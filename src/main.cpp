#include "application/observers.hpp"
#include "application/sensor_service.hpp"
#include "platform/mock_temperature_driver.hpp"
#include "sensor/filter.hpp"
#include "sensor/temperature_sensor.hpp"

#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

int main() {
    using namespace sensor_framework;

    // The mock driver represents an MCU-specific ADC/I2C/OneWire implementation.
    auto driver = std::make_unique<platform::MockTemperatureDriver>(
        std::vector<std::int32_t> {250, 252, 249, 400, 251, 253});
    auto sensor = std::make_unique<sensor::TemperatureSensor>(std::move(driver));
    application::SensorService service(std::move(sensor), std::make_unique<sensor::MovingAverageFilter>(3U));

    application::GuiObserver gui;
    application::MqttObserver mqtt;
    application::BusinessObserver business(30.0F);
    service.subscribe(gui);
    service.subscribe(mqtt);
    service.subscribe(business);

    if (!service.initialize()) {
        std::cerr << "Sensor initialization failed\n";
        return 1;
    }

    std::cout << "Filter: " << service.filter_name() << '\n';
    for (std::uint32_t time_ms = 0U; time_ms < 6000U; time_ms += 1000U) {
        service.sample_and_publish(time_ms);
    }

    std::cout << "\nSwitch filter at runtime: median\n";
    service.set_filter(std::make_unique<sensor::MedianFilter>(3U));
    for (std::uint32_t time_ms = 6000U; time_ms < 9000U; time_ms += 1000U) {
        service.sample_and_publish(time_ms);
    }

    return 0;
}
