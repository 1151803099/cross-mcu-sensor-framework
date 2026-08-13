#pragma once

#include "platform/temperature_driver.hpp"
#include "stm32f1_onewire.hpp"

#include <array>
#include <cstdint>

namespace sensor_framework::stm32f1 {

/**
 * @brief DS18B20 temperature-driver adapter for one externally powered device.
 *
 * This implementation uses SKIP_ROM, so it supports exactly one DS18B20 on
 * the configured bus. Multiple devices require ROM search and MATCH_ROM.
 */
class Stm32f1Ds18b20Driver final : public platform::ITemperatureDriver {
public:
    /**
     * @param port 1-Wire GPIO port, for example GPIOA.
     * @param pin 1-Wire GPIO pin, for example GPIO_PIN_1.
     * @param resolution_bits DS18B20 resolution from 9 to 12 bits; invalid values use 12 bits.
     */
    Stm32f1Ds18b20Driver(GPIO_TypeDef* port, std::uint16_t pin, std::uint8_t resolution_bits = 12U);

    /** @brief Detect one DS18B20 and verify its family code and ROM CRC. */
    bool initialize() override;

    /**
     * @brief Perform one synchronous conversion and read the scratchpad.
     * @param[out] raw_value DS18B20 signed 1/16 degree-Celsius value.
     * @return false on bus, family-code, or CRC error.
     * @note At 12-bit resolution this blocks for up to 750 ms. Call from a task/main loop, never an ISR.
     */
    bool read_raw(std::int32_t& raw_value) override;

    /** @brief Convert DS18B20 1/16 degree-Celsius raw data to Celsius. */
    float raw_to_celsius(std::int32_t raw_value) const override;

    /** @return Cached eight-byte DS18B20 ROM code after successful initialize(). */
    const std::array<std::uint8_t, 8U>& rom_code() const { return rom_code_; }

private:
    static constexpr std::uint8_t kFamilyCode = 0x28U;
    static constexpr std::uint8_t kReadRom = 0x33U;
    static constexpr std::uint8_t kSkipRom = 0xCCU;
    static constexpr std::uint8_t kConvertTemperature = 0x44U;
    static constexpr std::uint8_t kReadScratchpad = 0xBEU;

    static std::uint8_t crc8(const std::uint8_t* data, std::uint8_t length);
    std::uint32_t conversion_time_ms() const;

    Stm32f1OneWire wire_;
    std::array<std::uint8_t, 8U> rom_code_ {};
    std::uint8_t resolution_bits_;
};

}  // namespace sensor_framework::stm32f1

