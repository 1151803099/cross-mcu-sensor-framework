#include "stm32f1_onewire.hpp"

namespace sensor_framework::stm32f1 {

void Stm32f1OneWire::initialize() {
    GPIO_InitTypeDef config {};
    config.Pin = pin_;
    config.Mode = GPIO_MODE_OUTPUT_OD;
    config.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(port_, &config);
    release_bus();
}

bool Stm32f1OneWire::reset_and_detect() {
    drive_low();
    delay_us(480U);       // Reset low pulse: >= 480 us.
    release_bus();
    delay_us(70U);        // Sample within the DS18B20 presence-pulse window.
    const bool present = !read_bus();
    delay_us(410U);       // Finish the reset time slot before issuing a command.
    return present;
}

void Stm32f1OneWire::write_bit(bool bit) {
    drive_low();
    if (bit) {
        delay_us(6U);     // Write-1: release early.
        release_bus();
        delay_us(64U);
    } else {
        delay_us(60U);    // Write-0: hold low for the whole slot.
        release_bus();
        delay_us(10U);
    }
}

bool Stm32f1OneWire::read_bit() {
    drive_low();
    delay_us(3U);
    release_bus();
    delay_us(10U);        // DS18B20 data is valid about 15 us after slot start.
    const bool bit = read_bus();
    delay_us(55U);        // Complete the >= 60 us read time slot.
    return bit;
}

void Stm32f1OneWire::write_byte(std::uint8_t value) {
    for (std::uint8_t bit_index = 0U; bit_index < 8U; ++bit_index) {
        write_bit((value & 0x01U) != 0U);
        value >>= 1U;
    }
}

std::uint8_t Stm32f1OneWire::read_byte() {
    std::uint8_t value = 0U;
    for (std::uint8_t bit_index = 0U; bit_index < 8U; ++bit_index) {
        if (read_bit()) {
            value |= static_cast<std::uint8_t>(1U << bit_index);
        }
    }
    return value;
}

void Stm32f1OneWire::drive_low() {
    HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_RESET);
}

void Stm32f1OneWire::release_bus() {
    HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_SET);
}

bool Stm32f1OneWire::read_bus() const {
    return HAL_GPIO_ReadPin(port_, pin_) == GPIO_PIN_SET;
}

void Stm32f1OneWire::delay_us(std::uint32_t microseconds) const {
    // DWT is enabled by SystemInit on Cortex-M3 in a normal STM32CubeF1 project.
    // It provides deterministic timing unlike a compiler-dependent empty loop.
    const std::uint32_t start = DWT->CYCCNT;
    const std::uint32_t cycles = microseconds * (HAL_RCC_GetHCLKFreq() / 1000000U);
    while ((DWT->CYCCNT - start) < cycles) {
    }
}

}  // namespace sensor_framework::stm32f1

