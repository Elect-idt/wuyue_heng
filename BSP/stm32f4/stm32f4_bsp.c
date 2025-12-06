#include "stm32f4_bsp.h"
#include "bsp_debug_led.h"
#include "bsp_usart.h"

// stm32f4的所有外设操作接口，就是cpp里面抽象工厂派生类，里面包含了一组外设的方法工厂
const board_hw_bsp_t g_stm32f4_bsp_ = {.led_ops = &g_stm32f4_led_driver_,
                                       .usart_ops = &g_stm32f4_usart_driver_};
