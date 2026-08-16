#ifndef __BSP_INTERFACE_H_
#define __BSP_INTERFACE_H_

// 公共接口
#include "bsp_common_def.h"
#include "bsp_gpio_interface.h"
#include "bsp_usart_interface.h"
// #include "bsp_systick_interface.h"  ← FIX-05: SysTick 由 FreeRTOS 独占，不再作为公共 BSP ops 暴露
#include "bsp_spi_interface.h"

// 板级BSP描述符：聚合所有外设的驱动接口，实现平台无关的硬件抽象
// 每个外设成员（gpio_ops、usart_ops等）是该类外设的统一操作接口
// 外设大部分遵循RAII（资源获取即初始化），但关键系统资源需要在Bsp_Init中提前初始化
//
// 注意：SysTick 在 FreeRTOS 系统中属于 RTOS port 独占资源，不在此暴露。
// 微秒/毫秒级延时需求应使用 TIM 基础定时器或 DWT cycle counter。
//
// [C++设计模式对照] 借鉴抽象工厂模式(Abstract Factory)：
//   由于C没有继承，GoF中的抽象角色与具体角色坍缩为同一类型：
//
//   board_hw_bsp_t               → C++中 抽象工厂+具体工厂 的合并
//                                   C++需要两个类型(基类+派生类)，C中只需一个类型+不同实例
//   gpio_ops_t/usart_ops_t/...   → C++中 抽象产品+具体产品 的合并
//                                   C++需要多个派生类，
//                                   C中用id枚举在同一类型内区分不同实例
//   g_stm32f4_bsp_               → 具体工厂实例，函数指针指向该平台的实现（类似填好的虚表）
//   g_stm32f4_gpio_driver_/...   → 具体产品实例，函数指针指向该外设的实现（类似填好的虚表）
//
//   因此无需简单工厂和工厂方法：
//     产品为静态const实例，无需简单工厂来创建；工厂直接持有产品指针，无需工厂方法的CreateXxx()
typedef struct
{
    bsp_status_e (*platform_init)(void); // 平台级初始化（中断分组等全局配置）
    const gpio_ops_t *gpio_ops; // 指向GPIO操作方法的指针
    const usart_ops_t *usart_ops; // 指向USART操作方法的指针
    const spi_ops_t *spi_ops; // 指向SPI操作方法的指针
} board_hw_bsp_t;

extern const board_hw_bsp_t* g_board_hw_bsp_; // 全局单例对象

// 重要的资源需要一开始就初始化
extern bsp_status_e Bsp_Init(void);


#endif //__BSP_INTERFACE_H_
