#pragma once

#include "application/sensor_service.hpp"

#include <iostream>

namespace sensor_framework::application {

class GuiObserver final : public IReadingObserver {
public:
    /** @brief GUI 消费者示例；实际产品中此处更新 LVGL/显示模型。 */
    void on_reading(const SensorReading& reading) override {
        std::cout << "[GUI] " << reading.sensor_type << " = " << reading.value_celsius << " C\n";
    }
};

/**
 * @brief 云端数据适配示例。
 *
 * 本类只负责将读数映射为 topic 和 payload；真实 MQTT SDK 或 Wi-Fi 模组 UART
 * 传输应放在其后面的独立传输适配器中，避免传感器框架依赖具体云端实现。
 */
class MqttObserver final : public IReadingObserver {
public:
    /** @brief 将读数映射为 MQTT 发布信息；当前使用控制台模拟发送。 */
    void on_reading(const SensorReading& reading) override {
        std::cout << "[MQTT adapter] topic=device/sensors/" << reading.sensor_type
                  << ", payload={\"value_celsius\":" << reading.value_celsius
                  << ",\"timestamp_ms\":" << reading.timestamp_ms << "}\n";
    }
};

class BusinessObserver final : public IReadingObserver {
public:
    /** @param high_temperature_celsius 触发高温业务事件的阈值，单位摄氏度。 */
    explicit BusinessObserver(float high_temperature_celsius) : high_temperature_celsius_(high_temperature_celsius) {}

    /** @brief 超过阈值时执行业务事件；示例中打印日志。 */
    void on_reading(const SensorReading& reading) override {
        if (reading.value_celsius >= high_temperature_celsius_) {
            std::cout << "[Business] high-temperature event: " << reading.value_celsius << " C\n";
        }
    }

private:
    float high_temperature_celsius_; // 高温告警阈值。
};

}  // namespace sensor_framework::application
