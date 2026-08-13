#include "stm32f1_ds18b20_driver.hpp"

namespace sensor_framework::stm32f1 {

Stm32f1Ds18b20Driver::Stm32f1Ds18b20Driver(GPIO_TypeDef* port, std::uint16_t pin, std::uint8_t resolution_bits)
    : wire_(port, pin), resolution_bits_((resolution_bits >= 9U && resolution_bits <= 12U) ? resolution_bits : 12U) {}

bool Stm32f1Ds18b20Driver::initialize() {
    wire_.initialize();
    if (!wire_.reset_and_detect()) {
        return false;
    }

    wire_.write_byte(kReadRom);  // READ_ROM is valid only when one device is on the bus.
    for (std::uint8_t index = 0U; index < rom_code_.size(); ++index) {
        rom_code_[index] = wire_.read_byte();
    }
    return rom_code_[0] == kFamilyCode && crc8(rom_code_.data(), 7U) == rom_code_[7];
}

bool Stm32f1Ds18b20Driver::read_raw(std::int32_t& raw_value) {
    if (!wire_.reset_and_detect()) {
        return false;
    }
    wire_.write_byte(kSkipRom);
    wire_.write_byte(kConvertTemperature);
    HAL_Delay(conversion_time_ms());

    if (!wire_.reset_and_detect()) {
        return false;
    }
    wire_.write_byte(kSkipRom);
    wire_.write_byte(kReadScratchpad);

    std::array<std::uint8_t, 9U> scratchpad {};
    for (std::uint8_t index = 0U; index < scratchpad.size(); ++index) {
        scratchpad[index] = wire_.read_byte();
    }
    if (crc8(scratchpad.data(), 8U) != scratchpad[8]) {
        return false;
    }

    const std::int16_t signed_raw = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(scratchpad[0]) |
        (static_cast<std::uint16_t>(scratchpad[1]) << 8U));
    raw_value = signed_raw;
    return true;
}

float Stm32f1Ds18b20Driver::raw_to_celsius(std::int32_t raw_value) const {
    return static_cast<float>(raw_value) / 16.0F;
}

std::uint8_t Stm32f1Ds18b20Driver::crc8(const std::uint8_t* data, std::uint8_t length) {
    std::uint8_t crc = 0U;
    for (std::uint8_t data_index = 0U; data_index < length; ++data_index) {
        std::uint8_t value = data[data_index];
        for (std::uint8_t bit_index = 0U; bit_index < 8U; ++bit_index) {
            const std::uint8_t mix = (crc ^ value) & 0x01U;
            crc >>= 1U;
            if (mix != 0U) {
                crc ^= 0x8CU;  // Reversed polynomial x^8 + x^5 + x^4 + 1.
            }
            value >>= 1U;
        }
    }
    return crc;
}

std::uint32_t Stm32f1Ds18b20Driver::conversion_time_ms() const {
    switch (resolution_bits_) {
        case 9U: return 94U;
        case 10U: return 188U;
        case 11U: return 375U;
        default: return 750U;
    }
}

}  // namespace sensor_framework::stm32f1

