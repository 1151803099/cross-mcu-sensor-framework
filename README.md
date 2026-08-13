# Cross-MCU Sensor Framework

A small C++17 sensor framework demonstrating a temperature-sensor use case. It separates MCU-specific acquisition from sensor processing and independently delivers readings to GUI, cloud-transport, and business logic consumers.

## Architecture

```text
MCU ADC / I2C / OneWire driver
            |
            v
   ITemperatureDriver (platform adapter)
            |
            v
     TemperatureSensor (ISensor)
            |
            v
SensorService -> IFilter strategy -> SensorReading
            |                 |\
            v                 v v
       GUI observer      MQTT adapter   Business observer
```

### Responsibilities

- `platform/ITemperatureDriver`: the sole platform boundary. A GD32, ESP32, STM32, Linux I2C, or module-backed implementation only needs `initialize`, `read_raw`, and `raw_to_celsius`.
- `sensor/TemperatureSensor`: turns a raw platform reading into a temperature, with no application dependency.
- `sensor/IFilter`: Strategy pattern. `NoFilter`, `MovingAverageFilter`, and `MedianFilter` can be selected or switched at runtime.
- `application/SensorService`: owns the sampling pipeline and publishes a normalized `SensorReading`.
- `application/IReadingObserver`: Observer pattern. GUI, MQTT payload mapping, and business calculation are independent consumers and do not reference each other.

`MqttObserver` intentionally maps a reading to a topic/payload rather than owning a network stack. In production, its transport can use an MQTT client SDK on Linux/Wi-Fi SoC, or forward the payload over UART/SPI to a connectivity module. This keeps real-time MCU sensor/control code independent of a particular cloud SDK.

## Build and Run

```bash
cmake -S . -B build
cmake --build build
./build/sensor_demo
```

Run the framework tests with `ctest --test-dir build --output-on-failure`.

The demo uses `MockTemperatureDriver`; values are tenths of a degree Celsius and include an outlier to show filter behavior. Replace it with a concrete MCU driver without changing `TemperatureSensor`, filters, or application observers.

## Extension Points

- Add a new MCU: implement `ITemperatureDriver`.
- Add a sensor type: implement `ISensor`; reuse `SensorService`, filters, and observers.
- Add a filter: implement `IFilter`.
- Add a consumer: implement `IReadingObserver`.

## Embedded Notes

- The code avoids dynamic allocation in the periodic sampling path. Construction/setup may allocate; for constrained targets this can be replaced with static objects or a fixed allocator.
- A production scheduler invokes `sample_and_publish(timestamp_ms)` from an RTOS task or periodic bare-metal timer flag, not from an interrupt service routine.
- For concurrent RTOS use, subscription/filter changes should occur during initialization or be protected by the platform synchronization primitive.
