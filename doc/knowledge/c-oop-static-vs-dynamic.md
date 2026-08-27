# C 手动 OOP：静态注册表 vs 动态工厂

> 基础模式（vtable/抽象工厂/虚函数）的入门对照见 `doc/knowledge/cpp-vs-c-oop-pattern.md`，
> 本文件是进阶篇：什么时候该从"合一单例"升级到"虚表/实例分离"。

## 一、现状形态：虚表与实例合一（BSP 层，无状态）

`gpio_ops_t` 等 = **产品接口/策略**（不是工厂！判据：它的方法不返回产品，
它自己就是被 `->write()` 调用的产品）。`g_stm32f4_gpio_driver_` = const 具体产品。

`board_hw_bsp_t` = **抽象工厂的静态注册表变体**：引用预造好的 const 单例，
保留"整族配套切换"语义，砍掉创建动作。`bsp_interface.c` 的 `#if` = 选工厂。

**实例数据存哪？** 以 SPI 为例：`Bsp/stm32f4/spi/bsp_spi.c` 的
`s_spi_cfg[SPI_ID_MAX]`（`spi_hw_config_t` 数组，按 `spi_id` 索引）——
相当于把动态形态"每个实例一份的堆内存"换成"**编译期 const 数组的一行**"，
`id` 就是行下标。这就是静态形态能保持无状态函数 + 全 const 的原因：
状态没有消失，只是搬进了配置表。

嵌入式选它的理由：const 落 .rodata、零失败路径、产品天然单例（一块板就一套
GPIO 控制器）、派发链透明可 grep。

## 二、升级判据：需要第二个实例吗？

| 问题 | 是 → 升级动态形态 | 否 → 保持 const 单例 |
|---|---|---|
| 同类器件要同时存在 N 份？ | 3 组不同引脚的 74HC165 | 一个 GPIO 控制器 |
| 每实例要独立运行时状态？ | 各自的错误计数/配置 | 无状态纯操作 |
| 要运行时选择/销毁重建？ | 探测后装配、异常复位 | 编译期定死 |

**BSP 层保持 const 注册表不动；多实例需求出现在 Component 层。**

## 三、动态形态模板（虚表/实例分离 + create/destroy）

**触发案例（已定方向）**：将来做通用按键矩阵组件，同时驱动 3 组不同引脚的
74HC165，每组一个实例 -> 直接套本模板。

```c
/* 1. 虚表（= C++ class vtable，仍 const 全局一份）*/
typedef struct {
    bsp_status_e (*init)(struct keypad_dev *self);
    bsp_status_e (*scan)(struct keypad_dev *self, uint8_t *buf);
} keypad_vtable_t;

/* 2. 实例（= C++ object，堆/静态均可造 N 份，各带状态）*/
typedef struct keypad_dev {
    const keypad_vtable_t *vtable;   /* 每实例指向同一张虚表 */
    /* 配置区：创建时从 cfg 拷入，运行期只读 */
    const spi_ops_t *spi_ops;
    spi_id_e         spi_id;         /* 本实例绑定哪路 SPI */
    uint8_t          num_chips;      /* 本实例级联片数 */
    const bsp_lock_t *lock;
    /* 运行时状态区：各实例独立——这正是多实例化的意义 */
    uint32_t error_count;
} keypad_dev_t;

/* 创建配置（组合根用指定初始化器预填；字段与实例"配置区"一一对应）。
 * 为什么不直接预填 keypad_dev_t？——本器件有运行时状态（error_count），
 * 按 CLAUDE.md 判据"出现运行时状态时升级为独立 cfg 结构体"，配置与状态
 * 分居两个 struct，避免调用方碰到不该碰的状态字段（对照 hc165 的方案 A）*/
typedef struct {
    const spi_ops_t *spi_ops;
    spi_id_e         spi_id;
    uint8_t          num_chips;
    const bsp_lock_t *lock;
} keypad_cfg_t;

/* 3. 具体实现（每个方法第一参数都是 self，通过 self 取各自的配置/状态）*/
static bsp_status_e keypad_init_impl(struct keypad_dev *self)
{
    /* self->spi_id 是"本对象绑定哪路 SPI"（实例字段）；BSP 的 init(id) 是静态
     * 注册表寻址（id → s_spi_cfg[id] 配置行）。id 本质是静态形态下的
     * "穷人版对象句柄"——一个枚举值顶一个实例指针。若将来 BSP 也升级成
     * 虚表/实例形态，此处改写为 self->spi->vtable->init(self->spi)，id 消失 */
    return self->spi_ops->init(self->spi_id);
}
static bsp_status_e keypad_scan_impl(struct keypad_dev *self, uint8_t *buf)
{
    /* 完整器件事务：lock 包裹 + PL 脉冲 + CS + 读 num_chips 字节…… */
    return BSP_STAT_TRUE;
}

/* 4. 虚表本体：把实现"填表"，const、全局仅一份（= C++ 编译器生成的 __vtable）*/
static const keypad_vtable_t g_keypad_vtable = {
    .init = keypad_init_impl,
    .scan = keypad_scan_impl,
};

/* 5. 构造函数：手动版 C++ 构造函数（含编译器代写的那行）*/
keypad_dev_t *keypad_create(const keypad_cfg_t *cfg)
{
    keypad_dev_t *dev = pvPortMalloc(sizeof(*dev));
    if (!dev) return NULL;                 /* 动态形态新增的失败路径 */
    dev->vtable = &g_keypad_vtable;        /* ← C++ 里编译器自动干的这行 */
    dev->spi_ops   = cfg->spi_ops;         /* 配置区：从 cfg 逐字段拷入 */
    dev->spi_id    = cfg->spi_id;
    dev->num_chips = cfg->num_chips;
    dev->lock      = cfg->lock;
    dev->error_count = 0;                  /* 状态区：清零起步 */
    return dev;
}
/* 6. 配套 destroy()（防泄漏）；调用方: dev->vtable->scan(dev, buf) 每方法传 self */

/* 多实例并用：同一张虚表，N 个实例各走各的配置——升级的全部意义 */
keypad_dev_t *kp_left  = keypad_create(&(keypad_cfg_t){ .spi_id = SPI_ID_KP1, .num_chips = 3, ... });
keypad_dev_t *kp_right = keypad_create(&(keypad_cfg_t){ .spi_id = SPI_ID_KP2, .num_chips = 2, ... });
kp_left->vtable->scan(kp_left, buf1);    /* 走 KP1 的 SPI，与 kp_right 互不干扰 */
```

> 关键认知：**虚表一份（类级别），实例 N 份（对象级别）**——`g_keypad_vtable`
> 是"类"，`kp_left/kp_right` 是"对象"。这层分离正是 BSP 层 `ops_t`
> （虚表实例合一、单例够用）与本模板（虚表实例分离、多实例）的本质区别。

## 四、C 手动 vtable 与 C++ 的关系（脱糖对照）

`dev->vtable = &g_xxx_vtable;` 就是 C++ 构造函数里编译器**偷偷注入**的
`dev->__vptr = &Xxx::__vtable;` 的显式版。调用同链：

```
dev.write(pin)  --编译器脱糖-->  dev->__vptr->write(&dev, pin)
C 手写等价:                      dev->vtable->write(dev, pin)
```

差异四点：①C 忘填 vtable=空指针崩溃、填错表不报错（C++ 类型系统兜底）
②C++ 构造期间 vptr 指向当前类虚表（构造函数里调虚函数不到派生版，经典坑）
③C++ 虚表含 RTTI/多继承项，C 版没有 -> `.name` 字段就是人工 RTTI
④C 派发全程透明，代价是每次显式写 `dev->vtable->`

## 五、对象内存布局答疑（2026-08-17 追问沉淀）

**易混点**：看了 ops_t（函数指针是结构体成员）会误以为 C++ 虚函数声明也占对象
内存。实际 C++ 把"表"和"对象"分居两处：

```
多态对象（每个 new 一份）              类虚表（.rodata 全局一份）
┌───────────────────┐            ┌──────────────┐
│ vptr  @偏移0       │ ────────→  │ [0] on 指针   │ ← 函数指针住这里，
│ 数据成员（声明序）  │            │ [1] off 指针  │   不在对象里！
└───────────────────┘            └──────────────┘
```

- `sizeof(纯虚基类) = 4`（ARM32）= 仅 vptr；虚函数再多也不加（进表不进对象）。
  N 个对象共享 1 张表，每对象只付 4 字节——"虚表一份实例 N 份"的尺寸体现

```cpp
class LED {                    // 逐行对照：
public:
    virtual void on() = 0;     // ← 这是"声明"，不是成员！指针存到虚表里去了
    virtual void off() = 0;    // ← 同上
};                             // sizeof(LED) = 4，只有 vptr

class DimLED : public LED {
    int brightness;    // 数据成员
    bool enabled;
};
// sizeof = 12（ARM32）：vptr(4) + int(4) + bool(1) → 对齐补到 12
// 布局：[vptr @0][brightness @4][enabled @8][padding @9..11]
```
- **vptr 在偏移 0，数据成员随后**：这是"基类指针承诺"的物理基础，`LED*` 指
  任何派生对象都能在偏移 0 无条件取到 vptr
- **vptr 在对象里，不在指针里**：`LED* dev` 本身只有 4 字节地址；类型的作用
  是给编译器"所指物布局"的编译期描述
- **抽象类也有自己的 vtable**：纯虚槽指向 `__cxa_pure_virtual` 桩（调用即
  abort 带诊断），服务于构造期窗口；构造链走完被派生表覆盖
- **本项目的 C↔C++ 换算**（记住这个就不会再混）：

| 你们的 C | C++ 对应物 |
|---|---|
| `ops_t` 整个结构体 | 类的**虚表**（不是对象！） |
| `ops_t*` 指针 | 对象里的 **vptr** |
| `keypad_dev_t`（vtable 字段+数据） | **对象**，与 C++ 布局同构 |
| `g_xxx_driver_` 填好的表 | 编译器生成并填充的 `__vtable` |
| `hc165_init` 校验必填字段 | 编译器"禁止实例化抽象类"的人工替代 |

C 手动 OOP 的软肋：C++ 在编译期挡住"没实现完的类"，C 只能运行时校验兜底
（所以必填字段校验写进了 CLAUDE.md 惯例——用纪律补编译器缺的课）。

## 相关
- 架构现状与惯例：`.claude/memory/project-status.md`、CLAUDE.md 设计模式一节
- 入门对照：`doc/knowledge/cpp-vs-c-oop-pattern.md`
