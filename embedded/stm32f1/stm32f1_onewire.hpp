#pragma once

// This file is compiled by an STM32CubeF1 project, not by the host CMake demo.
#include "stm32f1xx_hal.h"

#include <cstdint>

namespace sensor_framework::stm32f1 {

/**
 * @brief STM32F103 HAL implementation of the physical 1-Wire bus.
 *
 * The data pin must have an external 4.7 kOhm pull-up to 3.3 V. This driver
 * uses open-drain output: driving high releases the bus; it never actively
 * drives a high logic level.
 */
class Stm32f1OneWire {
public:
    /** @param port GPIO port, for example GPIOA. @param pin GPIO pin, for example GPIO_PIN_1. */
    Stm32f1OneWire(GPIO_TypeDef* port, std::uint16_t pin) : port_(port), pin_(pin) {}

    /** @brief Configure the pin as open-drain output and release the bus. */
    void initialize();

    /** @return true when a 1-Wire device sends a presence pulse after reset. */
    bool reset_and_detect();

    /** @param bit Logic value to send, least significant bit first at byte level. */
    void write_bit(bool bit);
    /** @return Bit sampled from the bus. */
    bool read_bit();

    /** @brief Send one byte, least significant bit first as required by 1-Wire. */
    void write_byte(std::uint8_t value);
    /** @brief Read one byte, least significant bit first as required by 1-Wire. */
    std::uint8_t read_byte();

private:
    void drive_low();
    void release_bus();
    bool read_bus() const;
    void delay_us(std::uint32_t microseconds) const;

    GPIO_TypeDef* port_;
    std::uint16_t pin_;
};

}  // namespace sensor_framework::stm32f1

