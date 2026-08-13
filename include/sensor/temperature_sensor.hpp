#pragma once

#include "platform/temperature_driver.hpp"

#include <memory>
#include <utility>

namespace sensor_framework::sensor {

/** @brief 所有传感器对应用层提供的统一接口。 */
class ISensor {
public:
    virtual ~ISensor() = default;
    virtual bool initialize() = 0;
    /**
     * @brief 读取已经换算为工程单位的传感器值。
     * @param[out] value 读取成功时返回传感器数值。
     * @return 本次读取是否成功。
     */
    virtual bool read(float& value) = 0;
    virtual const char* type() const = 0;
};

class TemperatureSensor final : public ISensor {
public:
    /**
     * @brief 创建温度传感器对象。
     * @param driver 已实现 ITemperatureDriver 的平台驱动所有权。
     */
    explicit TemperatureSensor(std::unique_ptr<platform::ITemperatureDriver> driver)
        : driver_(std::move(driver)) {}

    /** @brief 初始化底层温度驱动。 */
    bool initialize() override { return driver_ != nullptr && driver_->initialize(); }

    /**
     * @brief 读取原始值并由驱动转换为摄氏温度。
     * @param[out] value 成功时写入摄氏温度。
     * @return 驱动不存在或读取失败时返回 false。
     */
    bool read(float& value) override {
        std::int32_t raw_value = 0;
        if (driver_ == nullptr || !driver_->read_raw(raw_value)) {
            return false;
        }
        value = driver_->raw_to_celsius(raw_value);
        return true;
    }

    /** @brief 返回传感器类型，用于 GUI 标签与云端 topic。 */
    const char* type() const override { return "temperature"; }

private:
    std::unique_ptr<platform::ITemperatureDriver> driver_;  // 平台相关实现仅保留在这里。
};

}  // namespace sensor_framework::sensor
