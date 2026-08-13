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

    // 1. 创建平台驱动。真实项目中替换为 GD32 ADC、STM32 I2C 或 Linux I2C 实现即可。
    // 样本单位为 0.1 摄氏度；400 即 40.0 C，用来模拟一次异常尖峰。
    auto driver = std::make_unique<platform::MockTemperatureDriver>(
        std::vector<std::int32_t> {250, 252, 249, 400, 251, 253});
    // 2. 用统一的温度传感器包装平台驱动；上层不再感知具体 MCU。
    auto sensor = std::make_unique<sensor::TemperatureSensor>(std::move(driver));
    // 3. 初始选用 3 点滑动平均滤波；3U 表示最多保留三个历史采样值。
    application::SensorService service(std::move(sensor), std::make_unique<sensor::MovingAverageFilter>(3U));

    // 4. 三个应用层订阅者互不依赖，只通过 SensorReading 接收处理结果。
    application::GuiObserver gui;
    application::MqttObserver mqtt;
    application::BusinessObserver business(30.0F);
    service.subscribe(gui);
    service.subscribe(mqtt);
    service.subscribe(business);

    // 5. 初始化底层驱动；失败时不进入周期采样。
    if (!service.initialize()) {
        std::cerr << "Sensor initialization failed\n";
        return 1;
    }

    std::cout << "Filter: " << service.filter_name() << '\n';
    // 6. 模拟每 1000 ms 一次的周期任务。实际 MCU 中由主循环 tick 或 RTOS 任务调用。
    for (std::uint32_t time_ms = 0U; time_ms < 6000U; time_ms += 1000U) {
        service.sample_and_publish(time_ms);
    }

    // 7. 不修改传感器和观察者代码，运行时替换为 3 点中值滤波。
    std::cout << "\nSwitch filter at runtime: median\n";
    service.set_filter(std::make_unique<sensor::MedianFilter>(3U));
    for (std::uint32_t time_ms = 6000U; time_ms < 9000U; time_ms += 1000U) {
        service.sample_and_publish(time_ms);
    }

    return 0;
}
