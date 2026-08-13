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

/**
 * @brief 一阶低通滤波器。
 *
 * 公式：y[n] = alpha * x[n] + (1 - alpha) * y[n-1]。
 * alpha 越小越平滑但响应越慢，alpha 越大响应越快但抑噪能力越弱。
 * 该实现只保存上一次输出，不需要历史数据容器。
 */
class LowPassFilter final : public IFilter {
public:
    /** @param alpha 平滑系数，自动限制在 [0, 1]。 */
    explicit LowPassFilter(float alpha) : alpha_(clamp_alpha(alpha)) {}

    /** @brief 输入一个新采样并计算低通输出。@param sample 当前未滤波采样值。 */
    float process(float sample) override {
        if (!initialized_) {
            output_ = sample;
            initialized_ = true;
        } else {
            output_ = alpha_ * sample + (1.0F - alpha_) * output_;
        }
        return output_;
    }

    /** @brief 清除历史输出，使下一次输入直接作为初始值。 */
    void reset() override {
        output_ = 0.0F;
        initialized_ = false;
    }

    const char* name() const override { return "low_pass"; }

    /** @brief 运行时修改平滑系数，修改后保留当前输出历史。 */
    void set_alpha(float alpha) { alpha_ = clamp_alpha(alpha); }
    float alpha() const { return alpha_; }

private:
    static float clamp_alpha(float alpha) { return std::clamp(alpha, 0.0F, 1.0F); }

    float alpha_;
    float output_ {0.0F};
    bool initialized_ {false};
};

/**
 * @brief 一维标量卡尔曼滤波器。
 *
 * 假设温度在相邻采样间没有控制输入，仅使用过程噪声 Q 和测量噪声 R
 * 估计当前值。Q/R 的相对大小决定响应速度与平滑程度。
 */
class KalmanFilter final : public IFilter {
public:
    /** @param process_noise_q 过程噪声 Q。@param measurement_noise_r 测量噪声 R。 */
    explicit KalmanFilter(float process_noise_q = 0.01F, float measurement_noise_r = 0.1F)
        : process_noise_q_(positive_or_default(process_noise_q)),
          measurement_noise_r_(positive_or_default(measurement_noise_r)) {}

    /** @brief 执行一次预测和测量更新。@param measurement 当前传感器测量值。 */
    float process(float measurement) override {
        if (!initialized_) {
            estimate_ = measurement;
            initialized_ = true;
            covariance_ = 1.0F;
            return estimate_;
        }

        covariance_ += process_noise_q_;  // 预测：不确定性随过程噪声增加。
        const float gain = covariance_ / (covariance_ + measurement_noise_r_);
        estimate_ += gain * (measurement - estimate_);
        covariance_ = (1.0F - gain) * covariance_;
        return estimate_;
    }

    void reset() override {
        estimate_ = 0.0F;
        covariance_ = 1.0F;
        initialized_ = false;
    }

    const char* name() const override { return "kalman"; }

    /** @brief 修改 Q/R 参数；非正值自动替换为最小正值。 */
    void set_noise_parameters(float process_noise_q, float measurement_noise_r) {
        process_noise_q_ = positive_or_default(process_noise_q);
        measurement_noise_r_ = positive_or_default(measurement_noise_r);
    }

    float estimate() const { return estimate_; }
    float covariance() const { return covariance_; }

private:
    static float positive_or_default(float value) { return value > 0.0F ? value : 0.001F; }

    float process_noise_q_;
    float measurement_noise_r_;
    float estimate_ {0.0F};
    float covariance_ {1.0F};
    bool initialized_ {false};
};

}  // namespace sensor_framework::sensor
