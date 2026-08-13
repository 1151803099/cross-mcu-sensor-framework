#pragma once

#include <cstdint>

namespace sensor_framework::platform {

/**
 * @brief 温度传感器的跨平台底层驱动接口。
 *
 * 移植到新的 MCU 时，只需实现本接口：例如 GD32 的 ADC、STM32 的 I2C
 * 或 Linux 下的 I2C 驱动。上层传感器、滤波和应用逻辑不需要修改。
 */
class ITemperatureDriver {
public:
    virtual ~ITemperatureDriver() = default;

    /** @brief 初始化硬件外设或传感器。@return 成功返回 true。 */
    virtual bool initialize() = 0;

    /**
     * @brief 读取未换算的原始数据。
     * @param[out] raw_value 驱动填入的原始 ADC 值、寄存器值或传感器原始值。
     * @return 本次读取是否成功。
     */
    virtual bool read_raw(std::int32_t& raw_value) = 0;

    /**
     * @brief 将原始数据转换为摄氏温度。
     * @param raw_value read_raw() 得到的原始值。
     * @return 换算后的温度，单位为摄氏度。
     */
    virtual float raw_to_celsius(std::int32_t raw_value) const = 0;
};

}  // namespace sensor_framework::platform
