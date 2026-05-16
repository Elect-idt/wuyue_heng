# C 实现 C++ 面向对象 — 本项目模式对照

## 一、核心映射关系

| C++ 概念 | C 实现 | 本项目对应 |
|----------|--------|-----------|
| 抽象类（纯虚函数） | 只含函数指针的 struct | `led_ops_t`、`usart_ops_t` |
| 具体类 | 函数实现 + 填充函数指针的 struct | `g_stm32f4_led_driver_` |
| 虚函数 | 函数指针 | `.on = stm32f4_led_on` |
| 虚函数表（vtable） | `*_ops_t` 结构体本身 | `led_ops_t` 就是 LED 的 vtable |
| 虚函数表指针（vptr） | 指向 `*_ops_t` 的指针 | `board_hw_bsp_t` 里的 `.led_ops` |
| 继承 | 不需要，C 用同一个类型 + 不同实例 | `led_ops_t` 既是抽象也是具体 |
| new / 构造函数 | 静态 const 实例 + 初始化 | `const led_ops_t g_stm32f4_led_driver_ = { ... }` |
| 抽象工厂 | 聚合多个 vtable 的 struct | `board_hw_bsp_t` |
| 具体工厂 | 填充好所有 vtable 指针的实例 | `g_stm32f4_bsp_` |
| 依赖注入 | 运行时赋值全局指针 | `g_board_hw_bsp_ = &g_stm32f4_bsp_` |

## 二、GoF 与"坍缩"

### GoF 是什么

**Gang of Four（四人帮）**——1994 年出版《设计模式》的四个作者：

- Erich Gamma
- Richard Helm
- Ralph Johnson
- John Vlissides

这本书定义了 23 个经典设计模式（包括抽象工厂、策略模式等）。软件工程里提 GoF 就是提这本书里的模式。

### "坍缩"是什么意思

**不是 GoF 坍缩，是 C 没有 C++ 的继承机制，导致 GoF 模式里的两种角色在 C 里合并成了一种。**

#### C++ 需要两个类型

```cpp
// GoF 定义的"抽象角色"——只声明接口，不实现
class LED {                       // 类型1：抽象产品
public:
    virtual void on() = 0;        // 纯虚函数（= 0 表示没有实现）
};

// GoF 定义的"具体角色"——继承抽象角色，提供实现
class STM32F4LED : public LED {   // 类型2：具体产品（继承自类型1）
    void on() override {          // 提供具体实现
        GPIO_SetBits(GPIOA, GPIO_Pin_5);
    }
};

// 两个不同的类型，编译器知道它们的继承关系
LED* led = new STM32F4LED();      // 基类指针 → 派生类对象
```

C++ 里"有哪些操作"和"操作怎么做"分在**两个类型**里。

#### C 只有一个类型

```c
// 没有 class，没有继承，没有 virtual
// "有哪些操作"和"操作怎么做"在同一个 struct 里

// 这个 struct 同时扮演了 GoF 的两个角色：
typedef struct {
    bsp_status_e (*on)(led_id_e id);   // 函数指针：既声明了"有这个操作"
} led_ops_t;                             // 又可以填充具体实现

// "抽象角色"：led_ops_t 类型本身（定义了有什么函数指针）
// "具体角色"：led_ops_t 的不同实例（函数指针指向不同实现）

const led_ops_t g_stm32f4_led_driver_ = { .on = stm32f4_led_on };   // 具体产品A
const led_ops_t g_stm32h7_led_driver_ = { .on = stm32h7_led_on };   // 具体产品B
```

#### 对比

```
C++ (GoF 标准实现):              C (本项目):

类型1: class LED { virtual }     led_ops_t（同一个类型）
类型2: class STM32F4LED : LED    led_ops_t 的不同实例
       ↕ 继承关系                 ↕ 没有继承，用同一类型的不同实例区分

两个类型，编译器保证签名匹配       一个类型，靠人保证签名正确
```

**"坍缩"就是：C++ 要两个类型才能做的事，C 用一个类型的不同实例就做了——两种角色合并（坍缩）成了一种。**

## 三、三种工厂模式对比

### 简单工厂（Simple Factory）— 一个函数根据参数创建

```cpp
// ===== C++ 版本 =====
class LED {
public:
    virtual void on() = 0;
    virtual ~LED() = default;
};

class DebugLED : public LED { void on() override { /* GPIOA Pin5 */ } };
class StatusLED : public LED { void on() override { /* GPIOB Pin6 */ } };

// 工厂：一个函数，根据参数决定创建哪个
class LEDFactory {
public:
    static LED* create(int id) {
        if (id == 0) return new DebugLED();
        if (id == 1) return new StatusLED();
        return nullptr;
    }
};

// 使用
LED* led = LEDFactory::create(0);  // 创建 DebugLED
led->on();
```

```c
// ===== C 版本 =====
typedef struct { void (*on)(int id); } led_ops_t;

led_ops_t g_debug_led  = { .on = debug_led_on };
led_ops_t g_status_led = { .on = status_led_on };

// 工厂：一个函数，根据参数返回不同实例
led_ops_t* led_factory(int id) {
    if (id == 0) return &g_debug_led;
    if (id == 1) return &g_status_led;
    return NULL;
}

// 使用
led_ops_t* led = led_factory(0);
led->on(0);
```

**特点**：一个函数搞定。**问题**：加新产品要改工厂函数里的 if-else。

---

### 工厂方法（Factory Method）— 每个产品由子类决定

```cpp
// ===== C++ 版本 =====
class LED {
public:
    virtual void on() = 0;
    virtual ~LED() = default;
};

// 工厂接口：定义创建方法，子类决定创建哪个
class LEDCreator {
public:
    virtual LED* create() = 0;   // 工厂方法
    virtual ~LEDCreator() = default;
};

// STM32F4 工厂
class STM32F4LEDCreator : public LEDCreator {
    LED* create() override { return new STM32F4LED(); }
};

// STM32H7 工厂
class STM32H7LEDCreator : public LEDCreator {
    LED* create() override { return new STM32H7LED(); }
};

// 使用
LEDCreator* factory = new STM32F4LEDCreator();
LED* led = factory->create();  // 创建 STM32F4LED
led->on();
```

```c
// ===== C 版本 =====
typedef struct { void (*on)(int id); } led_ops_t;

typedef struct {
    led_ops_t* (*create)(void);     // 工厂方法（函数指针）
} led_creator_t;

// STM32F4 工厂
led_ops_t* stm32f4_create_led(void) { return &g_stm32f4_led_driver_; }
led_creator_t g_stm32f4_led_creator = { .create = stm32f4_create_led };

// STM32H7 工厂
led_ops_t* stm32h7_create_led(void) { return &g_stm32h7_led_driver_; }
led_creator_t g_stm32h7_led_creator = { .create = stm32h7_create_led };

// 使用
led_creator_t* creator = &g_stm32f4_led_creator;
led_ops_t* led = creator->create();
led->on(0);
```

**特点**：加新产品不改旧代码，新增一个工厂就行。**问题**：各产品之间没有关联，LED 和 USART 可以混搭不同平台。

---

### 抽象工厂（Abstract Factory）— 创建一整族相关产品

```cpp
// ===== C++ 版本 =====
// 产品接口
class LED     { public: virtual void on(int id) = 0; virtual ~LED() = default; };
class USART   { public: virtual void send(int id, char c) = 0; virtual ~USART() = default; };
class SysTick { public: virtual void delay_ms(int ms) = 0; virtual ~SysTick() = default; };

// 抽象工厂：一族产品的创建接口
class BoardFactory {
public:
    virtual LED*     createLED()     = 0;
    virtual USART*   createUSART()   = 0;
    virtual SysTick* createSysTick() = 0;
    virtual void     platformInit()  = 0;
    virtual ~BoardFactory() = default;
};

// STM32F4 具体工厂：整族都是 STM32F4 的
class STM32F4Factory : public BoardFactory {
    LED*     createLED()     override { return new STM32F4LED(); }
    USART*   createUSART()   override { return new STM32F4USART(); }
    SysTick* createSysTick() override { return new STM32F4SysTick(); }
    void     platformInit()  override { NVIC_PriorityGroupConfig(Group4); }
};

// STM32H7 具体工厂：整族都是 STM32H7 的
class STM32H7Factory : public BoardFactory {
    LED*     createLED()     override { return new STM32H7LED(); }
    USART*   createUSART()   override { return new STM32H7USART(); }
    SysTick* createSysTick() override { return new STM32H7SysTick(); }
    void     platformInit()  override { /* H7 的 NVIC 配置 */ }
};

// 使用
BoardFactory* factory = new STM32F4Factory();   // 选择工厂
factory->platformInit();
LED* led = factory->createLED();
led->on(0);

// 切换芯片：只改这一行
factory = new STM32H7Factory();
```

```c
// ===== C 版本（本项目） =====
// 产品族：就是 vtable
typedef struct { bsp_status_e (*on)(led_id_e);            } led_ops_t;
typedef struct { bsp_status_e (*send)(usart_id_e, char);  } usart_ops_t;
typedef struct { bsp_status_e (*delay_ms)(systick_id_e, uint32_t); } systick_ops_t;

// 抽象工厂 + 具体工厂：同一个类型，不同实例
typedef struct {
    bsp_status_e (*platform_init)(void);
    const led_ops_t     *led_ops;       // vptr：指向 LED 的 vtable
    const usart_ops_t   *usart_ops;     // vptr：指向 USART 的 vtable
    const systick_ops_t *systick_ops;   // vptr：指向 SysTick 的 vtable
} board_hw_bsp_t;                       // 既是抽象工厂，也是具体工厂（坍缩）

// 具体工厂实例：STM32F4（整族都是 STM32F4 的）
const board_hw_bsp_t g_stm32f4_bsp_ = {
    .platform_init = stm32f4_platform_init,
    .led_ops       = &g_stm32f4_led_driver_,
    .usart_ops     = &g_stm32f4_usart_driver_,
    .systick_ops   = &g_stm32f4_systick_driver_,
};

// 具体工厂实例：STM32H7（整族都是 STM32H7 的）
const board_hw_bsp_t g_stm32h7_bsp_ = {
    .platform_init = stm32h7_platform_init,
    .led_ops       = &g_stm32h7_led_driver_,
    .usart_ops     = &g_stm32h7_usart_driver_,
    .systick_ops   = &g_stm32h7_systick_driver_,
};

// 使用
extern const board_hw_bsp_t* g_board_hw_bsp_;
g_board_hw_bsp_ = &g_stm32f4_bsp_;                           // 选择工厂
g_board_hw_bsp_->platform_init();                             // 平台初始化
g_board_hw_bsp_->led_ops->on(LED_DEBUG);                      // 调用 LED

// 切换芯片：只改这一行
g_board_hw_bsp_ = &g_stm32h7_bsp_;
```

**特点**：产品是一族的，不能混搭。**关键区别**：

```
简单工厂：  一个函数 → 多种 LED              （只管 LED）
工厂方法：  每种产品一个创建函数              （各管各的，可混搭）
抽象工厂：  一个工厂 → 一整族相关产品         （LED+USART+SYSTICK 打包，不可混搭）
```

### 判断本项目为什么是抽象工厂

| 判断条件 | 本项目 |
|---------|--------|
| 创建的是**一族**产品而不是单个？ | 是：led + usart + systick + spi 打包在 `board_hw_bsp_t` 里 |
| 产品之间有关联（不能混搭平台）？ | 是：STM32F4 的 USART 不能配 STM32H7 的 GPIO |
| 切换整个族只需换一个对象？ | 是：`g_board_hw_bsp_ = &g_stm32f4_bsp_` 一行搞定 |

## 四、虚函数表（vtable）详解

### C++ 编译器做的事（隐藏的）

```cpp
// 你写的 C++ 代码
class LED {
public:
    virtual void on(int id) = 0;
    virtual void off(int id) = 0;
    virtual ~LED() = default;
};

class STM32F4LED : public LED {
    void on(int id) override  { GPIO_SetBits(GPIOA, GPIO_Pin_5); }
    void off(int id) override { GPIO_ResetBits(GPIOA, GPIO_Pin_5); }
};
```

编译器在背后生成的等价代码：

```cpp
// 编译器自动生成的 vtable（隐藏的）
struct STM32F4LED_vtable {
    void (*on)(LED* this_, int id);     // 第1个虚函数
    void (*off)(LED* this_, int id);    // 第2个虚函数
    void (*destructor)(LED* this_);     // 第3个虚函数（析构）
};

// 编译器自动填充的 vtable 实例（隐藏的）
const STM32F4LED_vtable STM32F4LED_vtable_instance = {
    .on  = STM32F4LED::on,
    .off = STM32F4LED::off,
    .destructor = STM32F4LED::~STM32F4LED,
};

// 编译器给每个对象偷偷塞了一个 vptr（隐藏的）
class STM32F4LED : public LED {
    const STM32F4LED_vtable* vptr;   // ← 编译器偷偷加的，指向上面的 vtable
};
```

### 你手动做的事（C 代码，显式的）

```c
// bsp_led_interface.h — 你显式定义的 vtable
typedef struct {
    bsp_status_e (*init)(led_id_e id);     // 第1个"虚函数"
    bsp_status_e (*on)(led_id_e id);       // 第2个"虚函数"
    bsp_status_e (*off)(led_id_e id);      // 第3个"虚函数"
    bsp_status_e (*toggle)(led_id_e id);   // 第4个"虚函数"
} led_ops_t;

// bsp_debug_led.c — 你显式填充的 vtable 实例
const led_ops_t g_stm32f4_led_driver_ = {
    .init   = stm32f4_led_init,
    .on     = stm32f4_led_on,
    .off    = stm32f4_led_off,
    .toggle = stm32f4_led_toggle,
};
```

### 对比

```
C++ (编译器自动):                    C (你手动):
                                     ┌─────────────────────┐
编译器生成 vtable 结构    ←等价于→   │ typedef struct {     │
                                     │   (*on)(led_id_e);   │
                                     │   (*off)(led_id_e);  │
                                     │ } led_ops_t;         │
                                     └─────────────────────┘

编译器填充 vtable 实例    ←等价于→   const led_ops_t g_stm32f4_led_driver_ = {
                                     .on = stm32f4_led_on,
                                     .off = stm32f4_led_off,
                                     };

编译器塞 vptr 到对象里    ←等价于→   board_hw_bsp_t 里的 .led_ops 指针
对象->虚函数()             ←等价于→   g_board_hw_bsp_->led_ops->on(id)
```

## 五、完整对照表

```
C++ 抽象工厂                          C 本项目
─────────────────                     ─────────────────

class LED (抽象产品)           ←→     typedef struct { (*on)(); } led_ops_t（产品 vtable）
class STM32F4LED (具体产品)    ←→     const led_ops_t g_stm32f4_led_driver_ = { .on = ... }

class BoardFactory (抽象工厂)  ←→     typedef struct { *led_ops; *usart_ops; } board_hw_bsp_t
                                        （聚合所有产品 vtable 的"工厂 vtable"）

class STM32F4Factory (具体工厂)←→     const board_hw_bsp_t g_stm32f4_bsp_ = { .led_ops = &... }
                                        （填充好所有 vtable 指针的工厂实例）

BoardFactory* factory;         ←→     const board_hw_bsp_t* g_board_hw_bsp_;
                                        （全局工厂指针）

factory = new STM32F4Factory() ←→     g_board_hw_bsp_ = &g_stm32f4_bsp_;
                                        （依赖注入）

led = factory->createLED()     ←→     g_board_hw_bsp_->led_ops
                                        （通过工厂拿产品 vtable）

led->on(0)                     ←→     g_board_hw_bsp_->led_ops->on(LED_DEBUG)
                                        （虚函数调度）

factory = new STM32H7Factory() ←→     g_board_hw_bsp_ = &g_stm32h7_bsp_;
                                        （切换整族产品，一行搞定）
```

## 六、C 的优势和代价

### 优势
- **无隐藏开销**：C++ 的 vptr 是编译器偷偷塞的，C 的函数指针是你显式写的，完全可控
- **无虚函数调用开销**：实际上两者都是一次指针间接跳转，开销相同
- **可在 ROM 中**：`const led_ops_t` 放在 Flash 里，C++ 的 vtable 通常在 RAM
- **无 RTTI**：C 没有 `dynamic_cast` 和 `typeid`，不会引入额外代码体积

### 代价
- **手动管理**：C++ 编译器自动生成 vtable，C 要手写
- **没有编译期检查**：C++ 的 `override` 关键字能检查签名是否匹配，C 的函数指针赋值不做这个检查
- **没有析构函数**：C 没有 RAII，资源释放需要手动调用
