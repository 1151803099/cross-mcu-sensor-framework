# Submission Notes

## Design decisions

1. **Cross-MCU boundary**: `ITemperatureDriver` isolates each MCU's ADC, I2C, OneWire, or module API. The rest of the framework does not include MCU headers or touch hardware registers.
2. **Filtering**: `IFilter` is a Strategy interface. Filters can be selected during setup and replaced at runtime without changing sensor code or consumers.
3. **Application decoupling**: `SensorService` publishes the normalized result through `IReadingObserver`. GUI, cloud payload mapping, and business rules independently subscribe to the same reading.
4. **MQTT scope**: `MqttObserver` constructs a topic and payload only. The transport is intentionally injected below/behind this adapter so a product can use a Linux MQTT client, a Wi-Fi module SDK, or MCU-to-module UART without coupling the sensor framework to a vendor library.
5. **Embedded behavior**: periodic sampling is expected from a task or a main-loop timer flag. Hardware I/O and publishing are not performed in an ISR.

## Demo result

The demo uses samples `25.0, 25.2, 24.9, 40.0, 25.1, 25.3 C`, starts with a 3-point moving average, and then switches to a 3-point median filter. The three subscribers receive each processed result.

## Test coverage

`tests/framework_tests.cpp` verifies moving-average rolling behavior, median outlier rejection, raw-to-Celsius conversion, publish delivery, and runtime filter replacement.
