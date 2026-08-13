#include "application/sensor_service.hpp"
#include "platform/mock_temperature_driver.hpp"
#include "sensor/filter.hpp"
#include "sensor/temperature_sensor.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

namespace {

bool nearly_equal(float left, float right) {
    return std::fabs(left - right) < 0.001F;
}

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

class CapturingObserver final : public sensor_framework::application::IReadingObserver {
public:
    void on_reading(const sensor_framework::application::SensorReading& reading) override {
        count++;
        last_value = reading.value_celsius;
    }

    int count {0};
    float last_value {0.0F};
};

void test_filters() {
    using namespace sensor_framework::sensor;
    MovingAverageFilter average(3U);
    expect(nearly_equal(average.process(10.0F), 10.0F), "moving average: first sample");
    expect(nearly_equal(average.process(20.0F), 15.0F), "moving average: second sample");
    expect(nearly_equal(average.process(30.0F), 20.0F), "moving average: full window");
    expect(nearly_equal(average.process(40.0F), 30.0F), "moving average: rolling window");

    MedianFilter median(3U);
    median.process(20.0F);
    median.process(100.0F);
    expect(nearly_equal(median.process(21.0F), 21.0F), "median removes a single outlier");

    LowPassFilter low_pass(0.5F);
    expect(nearly_equal(low_pass.process(20.0F), 20.0F), "low-pass initializes from first sample");
    expect(nearly_equal(low_pass.process(30.0F), 25.0F), "low-pass applies alpha");
    low_pass.set_alpha(2.0F);
    expect(nearly_equal(low_pass.alpha(), 1.0F), "low-pass clamps alpha");
    expect(nearly_equal(low_pass.process(40.0F), 40.0F), "low-pass alpha one follows input");

    KalmanFilter kalman(0.01F, 0.1F);
    expect(nearly_equal(kalman.process(20.0F), 20.0F), "kalman initializes from first measurement");
    const float estimate = kalman.process(30.0F);
    expect(estimate > 20.0F && estimate < 30.0F, "kalman estimate stays between measurements");
    kalman.set_noise_parameters(0.0F, -1.0F);
    expect(kalman.covariance() >= 0.0F, "kalman keeps valid covariance after parameter clamp");
}

void test_service_publish_and_filter_switch() {
    using namespace sensor_framework;
    auto driver = std::make_unique<platform::MockTemperatureDriver>(std::vector<std::int32_t> {250, 270});
    auto temperature_sensor = std::make_unique<sensor::TemperatureSensor>(std::move(driver));
    application::SensorService service(std::move(temperature_sensor), std::make_unique<sensor::MovingAverageFilter>(2U));
    CapturingObserver observer;
    service.subscribe(observer);

    expect(service.initialize(), "sensor service initializes platform driver");
    expect(service.sample_and_publish(10U), "first sample is published");
    expect(nearly_equal(observer.last_value, 25.0F), "raw value converts to celsius");
    expect(service.sample_and_publish(20U), "second sample is published");
    expect(nearly_equal(observer.last_value, 26.0F), "filter processes sensor data");

    service.set_filter(std::make_unique<sensor::NoFilter>());
    expect(service.sample_and_publish(30U), "sampling continues after filter switch");
    expect(nearly_equal(observer.last_value, 25.0F), "runtime filter switch takes effect");
    expect(observer.count == 3, "observer receives every published reading");
}

}  // namespace

int main() {
    test_filters();
    test_service_publish_and_filter_switch();
    std::cout << "All tests passed\n";
    return 0;
}
