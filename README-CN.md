# 跨 MCU 温度 Sensor 框架


这是一个基于 C++17 的小型传感器框架示例，以温度传感器为案例，展示如何将底层 MCU 硬件、数据滤波和上层应用消费者解耦。

项目重点不是实现某一颗 MCU 的具体驱动，而是定义清晰的接口边界。将来从 GD32 切换到 STM32、ESP32 或 Linux 平台时，只需要替换底层驱动实现，不需要改动温度传感器、滤波器和应用逻辑。

## 题目对应关系

| 题目要求 | 本项目实现 |
| --- | --- |
| 跨 MCU 平台 | `ITemperatureDriver` 抽象 ADC、I2C、OneWire 等平台差异 |
| 温度 sensor 案例 | `TemperatureSensor` 负责读取和换算摄氏温度 |
| 多种滤波方式 | `NoFilter`、`MovingAverageFilter`、`MedianFilter` |
| GUI 解耦 | `GuiObserver` 订阅读数，不依赖传感器和滤波器 |
| MQTT 解耦 | `MqttObserver` 负责构造 topic/payload，网络传输可继续在其后适配 |
| 业务计算解耦 | `BusinessObserver` 根据温度阈值产生高温事件 |
| C++ 实现 | 使用接口、`unique_ptr`、策略模式、观察者模式和 C++17 标准库 |

## 总体架构

```text
实际硬件：ADC / I2C / OneWire 温度器件
                    |
                    v
         ITemperatureDriver（平台适配层）
                    |
                    v
        TemperatureSensor（温度传感器层）
                    |
                    v
           SensorService（采样编排层）
                    |
                    v
             IFilter（可替换滤波策略）
                    |
                    v
       SensorReading（统一温度数据结构）
          |                 |                 |
          v                 v                 v
   GuiObserver       MqttObserver      BusinessObserver
```

一次周期采样的顺序如下：

1. 定时任务或主循环调用 `SensorService::sample_and_publish()`。
2. `TemperatureSensor` 通过 `ITemperatureDriver` 读取底层原始值。
3. 驱动把原始 ADC 值或寄存器值换算成摄氏温度。
4. `SensorService` 把温度输入当前选中的滤波器。
5. `SensorService` 生成统一的 `SensorReading`，通知所有订阅者。
6. GUI、MQTT 数据适配和业务逻辑各自处理读数，彼此不直接调用。

## 目录说明

```text
include/
  platform/
    temperature_driver.hpp       跨 MCU 底层温度驱动接口
    mock_temperature_driver.hpp  桌面演示用模拟驱动
  sensor/
    temperature_sensor.hpp       温度传感器抽象
    filter.hpp                   滤波策略与三种滤波实现
  application/
    sensor_service.hpp           采样、滤波、发布的核心服务
    observers.hpp                GUI、MQTT、业务三个订阅者示例
src/
  main.cpp                       程序入口和完整演示流程
tests/
  framework_tests.cpp            基础单元测试
文件大纲.txt                      中文阅读顺序和调用链说明
SUBMISSION.md                    面试时可参考的设计决策说明
```

## 核心设计

### 1. 跨 MCU 平台设计

`ITemperatureDriver` 是唯一接触硬件的接口，包含：

- `initialize()`：初始化 ADC、I2C、GPIO 或具体温度传感器。
- `read_raw()`：读取原始采样值。
- `raw_to_celsius()`：将原始值转换为摄氏温度。

例如 GD32 平台可以实现 `Gd32AdcTemperatureDriver`，STM32 平台可以实现 `Stm32I2cTemperatureDriver`。它们都实现同一个接口，因此 `TemperatureSensor` 不需要知道当前使用哪颗 MCU、哪条总线或哪种温度器件。

这种设计的好处是：硬件变化被限制在平台层，业务和应用代码不跟随硬件变动，降低移植和维护成本。

### 2. 滤波策略设计

`IFilter` 是滤波器的统一接口，当前包含三种实现：

- `NoFilter`：不处理，直接输出当前采样值，适合调试或本身稳定的传感器。
- `MovingAverageFilter`：滑动窗口均值，适合抑制随机噪声，但对突变的响应会慢一些。
- `MedianFilter`：取窗口中值，适合抑制偶发的异常尖峰，例如 ADC 被瞬间干扰时出现的单点跳变。

`SensorService` 只知道当前对象实现了 `IFilter`，不知道具体算法。这是策略模式。运行时可以调用 `set_filter()` 切换滤波方式，而不用修改传感器或应用消费者代码。

### 3. GUI、MQTT 和业务逻辑解耦

`SensorService` 采样完成后不直接调用 GUI、网络或告警业务，而是把 `SensorReading` 发布给实现了 `IReadingObserver` 的订阅者。

- `GuiObserver`：实际产品中可以更新 LVGL 控件或显示模型。
- `MqttObserver`：本示例仅生成 MQTT topic 和 JSON payload。真实 MQTT client、TLS、重连等网络逻辑应放在独立传输适配层，或通过 UART/SPI 发给 Wi-Fi 模组。
- `BusinessObserver`：示例中用高温阈值产生事件；真实产品中可以扩展为风机控制、故障告警、数据记录等。

这种方式属于观察者模式或发布订阅模式。新增一个消费者时，只需新增一个观察者类并订阅，不需要改动 `SensorService`。

## 演示程序说明

`src/main.cpp` 中使用 `MockTemperatureDriver` 模拟温度数据：

```text
25.0, 25.2, 24.9, 40.0, 25.1, 25.3 摄氏度
```

其中 `40.0 C` 是人为放入的单次异常尖峰。程序先使用 3 点滑动平均滤波采样，再在运行时切换为 3 点中值滤波，便于观察不同滤波器的处理效果。

注意：`MockTemperatureDriver` 只是为了在 PC 上演示运行逻辑。量产代码中应替换为真实硬件驱动，其他上层代码保持不变。

## 构建与运行

本项目使用 CMake，要求编译器支持 C++17：

```bash
cmake -S . -B build
cmake --build build
./build/sensor_demo
```

运行测试：

```bash
ctest --test-dir build --output-on-failure
```

测试覆盖：滑动平均滤波、中值滤波、原始值转摄氏温度、读数发布，以及运行时切换滤波器。

## 面向资源受限 MCU 的落地说明

本示例优先展示可读性、可扩展性和可测试性，因此在对象创建阶段使用了 `unique_ptr` 和标准容器。周期采样路径本身不主动创建新的传感器或滤波对象。

对于 RAM/Flash 很小、长期运行且不希望使用堆的 MCU 量产项目，可在不改变总体架构的前提下进行以下调整：

- 用静态对象或引用注入替代 `unique_ptr`，避免堆分配。
- 用编译期固定长度的数组或环形缓冲区替代动态容器，固定滤波窗口和最大订阅者数量。
- 在初始化阶段完成订阅和滤波器选择，运行期间不改变配置。
- 由 RTOS 周期任务或主循环的定时标志调用采样函数，不在中断服务函数中执行复杂计算或网络发送。

当前 `SensorService` 保存的是非拥有型观察者指针：观察者对象由应用层创建和管理，必须在取消订阅前保持有效。示例中的三个观察者都在 `main()` 中创建，生命周期覆盖整个服务运行过程。

同一个观察者重复调用 `subscribe()` 时，服务会忽略重复订阅，保证一次采样只接收一次回调。实际产品中，观察者对象销毁前必须先调用 `unsubscribe()`，否则服务仍保存已失效地址，后续发布会产生未定义行为。

## 可扩展方向

- 新增 MCU：实现 `ITemperatureDriver`。
- 新增传感器：实现 `ISensor`，并继续复用现有滤波器和订阅者机制。
- 新增滤波算法：实现 `IFilter`，例如限幅滤波；当前已提供一阶低通滤波和一维卡尔曼滤波。
- 新增应用消费者：实现 `IReadingObserver`，例如本地存储、故障诊断、风机闭环控制。

### 当前滤波器参数

- 一阶低通：`alpha` 在 0 到 1 之间；值越小越平滑，值越大响应越快。DS18B20 可从 `0.3~0.5` 附近开始调试。
- 卡尔曼：`Q` 是过程噪声，`R` 是测量噪声；`Q/R` 越大通常响应越快，越小通常越平滑。参数应通过实际采样数据和阶跃响应验证，不能只凭经验值确定。

## STM32F103C8T6 + DS18B20 示例

项目已提供 `embedded/stm32f1/` 平台适配层，其中 `Stm32f1Ds18b20Driver` 实现了 `ITemperatureDriver`。因此接入 DS18B20 后，`TemperatureSensor`、滤波器和 GUI/MQTT/业务订阅者无需改变。

该目录的详细硬件连接、CubeMX 配置、DWT 微秒延时初始化、单设备限制和集成示例见 [embedded/stm32f1/README.md](embedded/stm32f1/README.md)。
