# STM32F103C8T6 + DS18B20 适配层

本目录是当前传感器框架在 STM32F103C8T6 上使用 DS18B20 的具体平台实现。它不加入根目录的 PC CMake 演示目标；应将以下四个文件加入 STM32CubeIDE/Keil 的 STM32F1 工程，并配置 STM32 HAL 头文件路径：

- `stm32f1_onewire.hpp/.cpp`：1-Wire 物理时序和字节读写。
- `stm32f1_ds18b20_driver.hpp/.cpp`：DS18B20 命令、ROM 检查、CRC 检查和温度转换。

## 硬件连接

```text
STM32F103C8T6 PA1 ---- DQ (DS18B20)
                         |
                        4.7 kOhm
                         |
                        3.3 V

STM32 3.3 V ---------- VDD (DS18B20)
STM32 GND ------------ GND (DS18B20)
```

当前实现针对**外部 3.3 V 供电**的单个 DS18B20，不支持寄生供电。DQ 必须有外部 4.7 kOhm 上拉；不要将 GPIO 配为推挽输出，否则从设备无法拉低总线响应。

## CubeMX / 工程配置

1. 配置系统时钟后，确保 `HAL_RCC_GetHCLKFreq()` 返回正确的 HCLK。典型值为 72 MHz。
2. 启用 `SYS -> Debug: Serial Wire`，保留 SWD 调试。
3. PA1 可先配置为 GPIO，但驱动的 `initialize()` 会将其重新配置为高速开漏输出。
4. 在 `main()` 早期启用 DWT 周期计数器，否则微秒延时无法运行：

```cpp
CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
DWT->CYCCNT = 0U;
DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
```

## 集成示例

`Stm32f1Ds18b20Driver` 已实现框架原有的 `ITemperatureDriver`，因此仍使用现有的 `TemperatureSensor` 与 `SensorService`：

```cpp
#include "stm32f1_ds18b20_driver.hpp"
#include "sensor/temperature_sensor.hpp"

using namespace sensor_framework;

// 示例用 unique_ptr 保持与当前框架构造方式一致。
auto driver = std::make_unique<stm32f1::Stm32f1Ds18b20Driver>(GPIOA, GPIO_PIN_1, 12U);
auto sensor = std::make_unique<sensor::TemperatureSensor>(std::move(driver));
```

随后将 `sensor` 传给 `SensorService` 即可。`initialize()` 会检测设备、读取 ROM 并验证 DS18B20 的家族码 `0x28` 和 ROM CRC。

## 时序与运行注意事项

- DS18B20 的 `read_raw()` 为同步实现：9/10/11/12 位分辨率分别最多等待 94/188/375/750 ms。
- 该函数只能由主循环或 RTOS 任务调用，禁止在中断中调用。
- 当前版本使用 `SKIP_ROM`，只适用于单个 DS18B20。多设备总线需要补充 `SEARCH_ROM`、保存 ROM ID，并改用 `MATCH_ROM`。
- scratchpad 的 9 字节已做 CRC 校验。总线无响应、ROM family code 不符或 CRC 错误均返回 `false`。
- 若产品需要不阻塞主循环，可将转换流程拆为 `start_conversion()` 和 `read_conversion_result()` 两个状态机步骤；当前框架的同步 `read()` 接口暂未扩展该能力。
