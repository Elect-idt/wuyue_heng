#include "main.h"

/**
  * @brief  外设初始化函数
  * @note   外设初始化，在初始化需要延时的外设
            后续在自己任务开始前在初始化，因为
            此时调度器还没有启动
  * @param  无
  * @retval 无
  */
static void System_Init(void)
{
    /* 主频168MHZ,对SysTick延时进行初始化 */
    SysTick_Init(168);
    /* 4位抢占优先级 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
}

int main(void)
{
    System_Init();
    Bsp_Init();
    AppTaskCreate();

    /* 正常不会执行到这里 */
    while (1)
    {
    }
}
