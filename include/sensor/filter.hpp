#pragma once

#include <algorithm>
#include <cstddef>
#include <deque>
#include <memory>
#include <vector>

namespace sensor_framework::sensor {

class IFilter {
public:
    virtual ~IFilter() = default;
    virtual float process(float sample) = 0;
    virtual void reset() = 0;
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
    explicit MovingAverageFilter(std::size_t window_size) : window_size_(std::max<std::size_t>(1U, window_size)) {}

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
    std::size_t window_size_;
    std::deque<float> samples_;
    float sum_ {0.0F};
};

class MedianFilter final : public IFilter {
public:
    explicit MedianFilter(std::size_t window_size) : window_size_(std::max<std::size_t>(1U, window_size)) {}

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
    std::size_t window_size_;
    std::deque<float> samples_;
};

}  // namespace sensor_framework::sensor

