# Component 层架构设计

## 一、Component 层定位

Component 层介于 Apps 和 Bsp_Interface 之间，职责是**器件协议抽象**——将多个 BSP 接口组合起来，实现具体硬件器件的完整操作协议。

```
Apps (FreeRTOS 任务)
  │  创建同步机制、注入到 Component
  ▼
Component (RTOS 无关)
  │  组合 gpio_ops_t + spi_ops_t 实现器件协议
  ▼
Bsp_Interface (平台无关接口)
  │  *_ops_t 函数指针表
  ▼
Bsp_Driver (STM32 平台实现)
```

### 为什么需要 Component 层？

| 没有 Component | 有 Component |
|---------------|-------------|
| Apps 直接调 gpio_ops + spi_ops 组合时序 | Apps 只调 `hc165_read()` 一行 |
| 每个使用 74HC165 的任务都要重复时序代码 | 时序封装在 Component，复用 |
| 器件协议散落在 Apps 层 | 器件协议集中管理 |

### Component 的核心约束

- **零个 FreeRTOS 头文件** — 纯 Bsp_Interface 依赖
- **RTOS 无关** — 同步机制通过函数指针从 Apps 注入
- **编译隔离** — 看不到 `stm32f4xx.h`（与 Apps 相同的隔离级别）

---

## 二、LED 器件

### 为什么 LED 是 Component 而不是 BSP？

LED 的"点亮"语义因电路而异：
- 低有效：LED_ON → GPIO_LOW（本项目，PA8）
- 高有效：LED_ON → GPIO_HIGH

这种**语义映射**不属于硬件操作（BSP），属于器件抽象（Component）。BSP 只提供 `gpio_ops_t`（set high/low/toggle），Component 负责映射。

### 设计

```c
// Component/led/led.h
typedef struct {
    const gpio_ops_t *gpio_ops;    // GPIO 操作接口
    gpio_pin_e       pin;          // GPIO 引脚 ID
    bool             active_low;   // true: 低电平点亮
} led_t;

void led_init(led_t *led, const gpio_ops_t *ops, gpio_pin_e pin, bool active_low);
void led_on(led_t *led);
void led_off(led_t *led);
void led_toggle(led_t *led);
```

### active_low 映射

```
                  active_low=true          active_low=false
led_on()    →    gpio_write(LOW)     →    gpio_write(HIGH)
led_off()   →    gpio_write(HIGH)    →    gpio_write(LOW)
led_toggle()→    gpio_write(TOGGLE)  →    gpio_write(TOGGLE)
```

---

## 三、74HC165 移位寄存器

### 硬件连接

| 74HC165 引脚 | STM32 引脚 | BSP 接口 | 说明 |
|-------------|-----------|---------|------|
| PL (并行加载) | PB1 | `gpio_ops_t` | LOW 锁存，HIGH 移位 |
| CE (时钟使能) | PB12 | `spi_ops_t.spi_cs_control` | LOW 使能时钟 |
| SCK (时钟) | PB13 | SPI2_SCK (AF) | 时钟自动产生 |
| Q7 (串行输出) | PB14 | SPI2_MISO (AF) | 数据输出到 MCU |
| SER (串行输入) | PB15 | SPI2_MOSI (AF) | 发送 dummy 产生时钟 |

### 读时序

```
┌───┐                           ┌───┐
│   │                           │   │
│   └───────────────────────────┘   │
PL  LOW                         HIGH
    ↓锁存并行输入                  ↓进入移位模式

        ┌───┐                                           ┌───┐
        │   │                                           │   │
────────┘   └───────────────────────────────────────────┘   └────
CE      LOW                                             HIGH
        ↓使能SPI时钟                                   ↓结束
        │←──── SPI DMA 读取 N 字节 ────→│
```

**完整步骤：**
1. `gpio_ops->write(pl_pin, GPIO_LOW)` — PL LOW，锁存并行输入
2. `gpio_ops->write(pl_pin, GPIO_HIGH)` — PL HIGH，进入移位模式
3. `spi_ops->spi_cs_control(id, ENABLE)` — CE LOW，使能 SPI 时钟
4. `spi_ops->spi_receive_multi_data_dma(...)` — DMA 读取 N 字节
5. `spi_ops->spi_cs_control(id, DISABLE)` — CE HIGH，结束

### 器件描述符

```c
// Component/74hc165/74hc165.h
typedef struct {
    const spi_ops_t      *spi_ops;    // SPI 操作接口（CE + 数据传输）
    spi_id_e             spi_id;      // SPI 设备 ID
    const spi_dma_sync_t *dma_sync;   // DMA 同步（Apps 注入，NULL 则用轮询）
    uint8_t              num_chips;   // 级联芯片数量
    const gpio_ops_t     *gpio_ops;   // GPIO 接口（PL 控制）
    gpio_pin_e           pl_pin;      // PL 引脚 ID
} hc165_t;
```

### 依赖注入全景

```
Apps (key_scan_app.c)
  │
  │  xSemaphoreCreateBinary()
  │  包装为 spi_dma_sync_t { .handle=sem, .wait=xSemaphoreTake, .notify=xSemaphoreGiveFromISR }
  │
  ▼
hc165_init(&dev, spi_ops, id, &dma_sync, 3, gpio_ops, GPIO_PIN_HC165_PL)
  │
  │  dev.spi_ops   = spi_ops        ← 第1层注入：ops_t 实例
  │  dev.gpio_ops  = gpio_ops       ← 第1层注入：ops_t 实例
  │  dev.dma_sync  = &dma_sync      ← 第1层注入：同步机制
  │
  ▼
hc165_read() / hc165_read_polling()
  │  调用 spi_ops->spi_cs_control()     ← 通过函数指针间接调用
  │  调用 gpio_ops->write()              ← 通过函数指针间接调用
  │  调用 spi_ops->spi_receive_multi_data_dma(&dma_sync)
  │      └→ BSP 保存 sync 指针 → DMA ISR 调用 sync->notify_from_isr()
  │         → xSemaphoreGiveFromISR → 唤醒任务
```

---

## 四、CMake 编译隔离

```cmake
# Component/CMakeLists.txt
add_library(component_lib STATIC)

# 源文件
aux_source_directory(${CMAKE_SOURCE_DIR}/Component/led Component_Src)
aux_source_directory(${CMAKE_SOURCE_DIR}/Component/74hc165 Component_Src)

# PUBLIC：Apps 包含 74hc165.h / led.h 时能找到
target_include_directories(component_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/Component/led
    ${CMAKE_SOURCE_DIR}/Component/74hc165
)

# 只依赖 Bsp_Driver（通过 PUBLIC 传播获得 Bsp_Interface 头文件路径）
target_link_libraries(component_lib PUBLIC Bsp_Driver)
```

### 编译隔离验证

编译 `74hc165.c` 时的 include 路径：
- `-IComponent/74hc165` ✓
- `-IBsp/common` ✓
- `-IBsp` ✓
- `-IDrivers/...` ✗（看不到 STM32 SPL）
- `-IFreeRTOS/...` ✗（看不到 RTOS）

---

## 五、GPIO 驱动设计

### `gpio_ops_t` 接口

```c
typedef struct {
    const char *name;
    bsp_status_e (*init)(gpio_pin_e pin);              // 按引脚单独初始化
    bsp_status_e (*write)(gpio_pin_e pin, gpio_state_e state);  // HIGH/LOW/TOGGLE
} gpio_ops_t;
```

### 为什么 `init(pin)` 而不是 `init(void)`

- 每个引脚有独立完整配置（mode, speed, otype, pupd），不支持共享默认值
- 避免一个 IO 被其他 Component 重复初始化覆盖状态
- 后续引脚可能需要不同配置（如 I2C SDA 要开漏输出）

### 当前引脚映射

| `gpio_pin_e` | 物理引脚 | 默认状态 | 配置 |
|-------------|---------|---------|------|
| `GPIO_PIN_LED_STATUS` | PA8 | HIGH（熄灭） | 推挽输出，上拉 |
| `GPIO_PIN_HC165_PL` | PB1 | HIGH（移位模式） | 推挽输出，上拉 |
