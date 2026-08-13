#pragma once

#include <algorithm>
#include <cstddef>
#include <deque>
#include <memory>
#include <vector>

namespace sensor_framework::sensor {

/**
 * @brief 可替换的滤波策略接口。
 *
 * SensorService 只依赖本接口，因此新增滤波算法不会影响传感器或应用层。
 */
class IFilter {
public:
    virtual ~IFilter() = default;
    /** @param sample 本次未滤波输入。@return 本次滤波输出。 */
    virtual float process(float sample) = 0;
    /** @brief 清除历史采样数据。 */
    virtual void reset() = 0;
    /** @brief 返回滤波器名称，便于日志和诊断。 */
    virtual const char* name() const = 0;
};

class NoFilter final : public IFilter {
public:
    float process(float sample) override { return sample; }
    void reset() override {}
    const char* name() const override { return "none"; }
};

class MovingAverageFilter final : public IFilter {
public:
    /** @param window_size 滑动窗口长度；传入 0 时自动修正为 1。 */
    explicit MovingAverageFilter(std::size_t window_size) : window_size_(std::max<std::size_t>(1U, window_size)) {}

    /**
     * @brief 计算窗口内样本均值。
     * @param sample 当前原始采样值。
     * @return 最近 window_size_ 个样本的算术平均值。
     */
    float process(float sample) override {
        samples_.push_back(sample);
        sum_ += sample;
        if (samples_.size() > window_size_) {
            sum_ -= samples_.front();
            samples_.pop_front();
        }
        return sum_ / static_cast<float>(samples_.size());
    }

    void reset() override {
        samples_.clear();
        sum_ = 0.0F;
    }

    const char* name() const override { return "moving_average"; }

private:
    std::size_t window_size_;      // 最大保留样本数。
    std::deque<float> samples_;    // 先进先出的历史样本窗口。
    float sum_ {0.0F};             // 窗口总和，避免每次重新遍历求和。
};

class MedianFilter final : public IFilter {
public:
    /** @param window_size 中值滤波窗口长度；传入 0 时自动修正为 1。 */
    explicit MedianFilter(std::size_t window_size) : window_size_(std::max<std::size_t>(1U, window_size)) {}

    /**
     * @brief 计算窗口中值，可抑制单次尖峰干扰。
     * @param sample 当前原始采样值。
     * @return 窗口排序后的中间值；偶数个样本时取中间两个的平均值。
     */
    float process(float sample) override {
        samples_.push_back(sample);
        if (samples_.size() > window_size_) {
            samples_.pop_front();
        }

        std::vector<float> ordered(samples_.begin(), samples_.end());
        std::sort(ordered.begin(), ordered.end());
        const std::size_t middle = ordered.size() / 2U;
        return (ordered.size() % 2U == 0U) ? (ordered[middle - 1U] + ordered[middle]) / 2.0F : ordered[middle];
    }

    void reset() override { samples_.clear(); }
    const char* name() const override { return "median"; }

private:
    std::size_t window_size_;   // 最大保留样本数。
    std::deque<float> samples_; // 历史样本窗口。
};

}  // namespace sensor_framework::sensor
