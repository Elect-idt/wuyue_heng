#include "main.h"

/* 片上外设基地址 */
/* 总线基地址 */
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x00020000)
/*GPIO 外设基地址 */
#define GPIOA_BASE (AHB1PERIPH_BASE + 0x0000)
/* GPIOH 寄存器地址, 强制转换成指针 */
#define GPIOA_MODER *(unsigned int*)(GPIOA_BASE+0x00)
#define GPIOA_OTYPER *(unsigned int*)(GPIOA_BASE+0x04)
#define GPIOA_OSPEEDR *(unsigned int*)(GPIOA_BASE+0x08)
#define GPIOA_PUPDR *(unsigned int*)(GPIOA_BASE+0x0C)
#define GPIOA_IDR *(unsigned int*)(GPIOA_BASE+0x10)
#define GPIOA_ODR *(unsigned int*)(GPIOA_BASE+0x14)
#define GPIOA_BSRR *(unsigned int*)(GPIOA_BASE+0x18)
#define GPIOA_LCKR *(unsigned int*)(GPIOA_BASE+0x1C)
#define GPIOA_AFRL *(unsigned int*)(GPIOA_BASE+0x20)
#define GPIOA_AFRH *(unsigned int*)(GPIOA_BASE+0x24)
/*RCC 外设基地址 */
#define RCC_BASE (AHB1PERIPH_BASE + 0x3800)
/*RCC 的 AHB1 时钟使能寄存器地址, 强制转换成指针 */
#define RCC_AHB1ENR *(unsigned int*)(RCC_BASE+0x30)

void Delay(__IO uint32_t nCount) //简单的延时函数
{
    for (; nCount != 0; nCount--);
}

int main(void)
{   
    /* 开启 GPIOA 时钟，使用外设时都要先开启它的时钟 */
    RCC_AHB1ENR |= (1<<0);  
    /* LED 端口初始化 */    
    /*GPIOA MODER8 清空 */
    GPIOA_MODER &= ~( 0x03<< (2*8));
    /*PA8 MODER8 = 01b 输出模式 */

    GPIOA_MODER |= (1<<2*8);    
    /*GPIOA OTYPER8 清空 */
    GPIOA_OTYPER &= ~(1<<1*8);
    /*PA8 OTYPER8 = 0b 推挽模式 */
    GPIOA_OTYPER |= (0<<1*8);   
    /*GPIOA OSPEEDR8 清空 */
    GPIOA_OSPEEDR &= ~(0x03<<2*8);
    /*PA8 OSPEEDR8 = 0b 速率 2MHz*/
    GPIOA_OSPEEDR |= (0<<2*8);  
    /*GPIOA PUPDR8 清空 */
    GPIOA_PUPDR &= ~(0x03<<2*8);
    /*PA8 PUPDR8 = 01b 上拉模式 */
    GPIOA_PUPDR |= (1<<2*8);    
    /*PA8 BSRR 寄存器的 BR8 置 1，使引脚输出低电平 */
    GPIOA_BSRR |= (1<<16<<8);   
    /*PA8 BSRR 寄存器的 BS6 置 1，使引脚输出高电平 */
    GPIOA_BSRR |= (1<<8);

    while(1)
    {
        GPIOA_BSRR |= (1<<16<<8); 
        Delay(0xFFFFF);
        GPIOA_BSRR |= (1<<8);
        Delay(0xFFFFF);
    }
}
