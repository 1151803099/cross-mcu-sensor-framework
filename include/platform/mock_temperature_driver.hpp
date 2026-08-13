#pragma once

#include "platform/temperature_driver.hpp"

#include <cstddef>
#include <utility>
#include <vector>

namespace sensor_framework::platform {

class MockTemperatureDriver final : public ITemperatureDriver {
public:
    /**
     * @brief 构造桌面演示用的模拟温度驱动。
     * @param samples 原始温度序列；本例约定单位为 0.1 摄氏度。
     */
    explicit MockTemperatureDriver(std::vector<std::int32_t> samples) : samples_(std::move(samples)) {}

    /** @brief 模拟硬件初始化；序列非空即初始化成功。 */
    bool initialize() override { return !samples_.empty(); }

    /**
     * @brief 循环取出下一笔模拟原始温度。
     * @param[out] raw_value 返回当前样本。
     * @return 无样本时返回 false，否则返回 true。
     */
    bool read_raw(std::int32_t& raw_value) override {
        if (samples_.empty()) {
            return false;
        }
        raw_value = samples_[next_sample_++ % samples_.size()];
        return true;
    }

    /** @brief 将 0.1 摄氏度单位的原始值换算为摄氏度。 */
    float raw_to_celsius(std::int32_t raw_value) const override {
        return static_cast<float>(raw_value) / 10.0F;
    }

private:
    std::vector<std::int32_t> samples_;  // 模拟传感器采样数据。
    std::size_t next_sample_ {0U};       // 下次读取的样本下标。
};

}  // namespace sensor_framework::platform
