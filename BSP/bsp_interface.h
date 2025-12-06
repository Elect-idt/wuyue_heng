#ifndef __BSP_INTERFACE_H_
#define __BSP_INTERFACE_H_

#include <stdio.h>

// LED公共接口
#include "bsp_common_def.h"
#include "bsp_led_interface.h"
#include "bsp_usart_interface.h"

// 声明全局外设操作接口，就是cpp里面抽象工厂基类，包含了一组外设的方法工厂
// 这些外设遵循RAII 资源获取即初始化
typedef struct
{
    const led_ops_t *led_ops; // 指向LED操作方法的指针
    const usart_ops_t *usart_ops; // 指向LED操作方法的指针
} board_hw_bsp_t;

extern const board_hw_bsp_t* g_board_hw_bsp_; // 全局单例对象

// 重要的资源需要一开始就初始化
extern bsp_status_e Bsp_Init(void);


#endif //__BSP_INTERFACE_H_
