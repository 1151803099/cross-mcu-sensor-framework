#pragma once

#include "sensor/filter.hpp"
#include "sensor/temperature_sensor.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace sensor_framework::application {

/** @brief 传感器服务向所有应用消费者发布的标准数据结构。 */
struct SensorReading {
    const char* sensor_type;        // 传感器类型，例如 "temperature"。
    float value_celsius;            // 已经滤波后的温度，单位摄氏度。
    std::uint32_t timestamp_ms;     // 系统时间戳，单位毫秒。
};

/** @brief 应用层订阅接口，采用观察者模式实现消费者解耦。 */
class IReadingObserver {
public:
    virtual ~IReadingObserver() = default;
    /** @param reading 由 SensorService 构造并发布的标准读数。 */
    virtual void on_reading(const SensorReading& reading) = 0;
};

/**
 * @brief 温度采样、滤波与发布的编排服务。
 *
 * 每个周期依次完成：读取传感器 -> 滤波 -> 向所有订阅者发布。
 * GUI、MQTT 和业务逻辑只订阅读数，彼此没有调用依赖。
 */
class SensorService {
public:
    /**
     * @param sensor 传感器对象所有权。
     * @param filter 当前使用的滤波策略所有权。
     */
    SensorService(std::unique_ptr<sensor::ISensor> sensor, std::unique_ptr<sensor::IFilter> filter)
        : sensor_(std::move(sensor)), filter_(std::move(filter)) {}

    /** @brief 检查依赖并初始化底层传感器。@return 初始化是否成功。 */
    bool initialize() { return sensor_ != nullptr && filter_ != nullptr && sensor_->initialize(); }

    /**
     * @brief 在运行时切换滤波策略。
     * @param filter 新滤波器所有权；空指针会被忽略，保留旧滤波器。
     */
    void set_filter(std::unique_ptr<sensor::IFilter> filter) {
        if (filter != nullptr) {
            filter_ = std::move(filter);
        }
    }

    /** @brief 返回当前滤波器名称。 */
    const char* filter_name() const { return filter_ == nullptr ? "unavailable" : filter_->name(); }

    /**
     * @brief 新增读数订阅者；重复订阅同一对象不会重复保存。
     * @param observer GUI、MQTT 或业务观察者对象。
     * @note 本服务不拥有 observer。应用层必须保证 observer 在订阅期间有效，
     *       并在销毁前调用 unsubscribe()。
     */
    void subscribe(IReadingObserver& observer) {
        const auto existing = std::find(observers_.begin(), observers_.end(), &observer);
        if (existing == observers_.end()) {
            observers_.push_back(&observer);
        }
    }

    /** @brief 取消一个订阅者。@param observer 要移除的观察者对象。 */
    void unsubscribe(IReadingObserver& observer) {
        observers_.erase(std::remove(observers_.begin(), observers_.end(), &observer), observers_.end());
    }

    /**
     * @brief 执行一次完整采样流程。
     * @param timestamp_ms 本次采样时刻，通常来自系统 tick，单位毫秒。
     * @return 读取并完成发布返回 true；传感器/滤波器不可用或读取失败返回 false。
     */
    bool sample_and_publish(std::uint32_t timestamp_ms) {
        float raw_celsius = 0.0F;
        if (sensor_ == nullptr || filter_ == nullptr || !sensor_->read(raw_celsius)) {
            return false;
        }

        // 统一将原始温度变为已滤波的标准读数，再一次性通知各应用层消费者。
        const SensorReading reading {sensor_->type(), filter_->process(raw_celsius), timestamp_ms};
        for (IReadingObserver* observer : observers_) {
            observer->on_reading(reading);
        }
        return true;
    }

private:
    std::unique_ptr<sensor::ISensor> sensor_;            // 与具体传感器交互。
    std::unique_ptr<sensor::IFilter> filter_;            // 可被替换的滤波策略。
    // 非拥有型订阅者指针：对象生命周期由应用层管理，销毁前必须取消订阅。
    std::vector<IReadingObserver*> observers_;
};

}  // namespace sensor_framework::application
