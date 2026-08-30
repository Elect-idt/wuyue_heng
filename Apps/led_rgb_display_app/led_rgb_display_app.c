/**
 ******************************************************************************
 * @file    led_rgb_display_app.c
 * @author  Pan
 * @brief   rgb led 灯光任务（18颗WS2812级联，20ms周期刷新=50fps）
 ******************************************************************************
 */

#include "led_rgb_display_app.h"
#include "FreeRTOS.h"
#include "bsp_interface.h"
#include "semphr.h"
#include "task.h"
#include "ws2812_led.h"
#include <stdint.h>
#include <stdio.h>

#define LED_RGB_DISPLAY_PERIOD_MS 20
#define LED_RGB_DISPLAY_NUM 18
#define LED_RGB_DISPLAY_PRINT_INTERVAL 200 /* 打印间隔 ms，避免刷屏 */

static ws2812_led_t s_ws2812_led;
/* DMA 同步（FreeRTOS 信号量注入 Component） */
static SemaphoreHandle_t s_spi_dma_sem;
static spi_dma_sync_t s_spi_dma_sync;

/* 灯效帧缓冲：放哪都行（组件内部会拷贝到自己的 static 编码缓冲再 DMA，
 * 见 ws2812_led.h 契约）；用 static 只是为了避免每周期重算内容丢失 */
static ws2812_color_t s_led_rgb_frame[LED_RGB_DISPLAY_NUM] = {0};

static bool spi_dma_wait(void *handle, uint32_t timeout_ms)
{
    return xSemaphoreTake((SemaphoreHandle_t)handle, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

static void spi_dma_notify_from_isr(void *handle)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR((SemaphoreHandle_t)handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void Led_RGB_Display_Task(void *param)
{
    static TickType_t PreviousWakeTime;
    PreviousWakeTime = xTaskGetTickCount();

    /* 创建 DMA 同步信号量 */
    s_spi_dma_sem = xSemaphoreCreateBinary();
    configASSERT(s_spi_dma_sem != NULL);
    s_spi_dma_sync.handle = s_spi_dma_sem;
    s_spi_dma_sync.wait = spi_dma_wait;
    s_spi_dma_sync.notify_from_isr = spi_dma_notify_from_isr;

    /* 初始化 WS2812：预填描述符（指定初始化器，字段自文档），再校验+硬件 init */
    s_ws2812_led = (ws2812_led_t){
        .spi_ops = g_board_hw_bsp_->spi_ops,
        .spi_id = SPI_ID_WS2812_LED,
        .dma_sync = &s_spi_dma_sync, /* NULL = 轮询模式 */
        .num_leds = LED_RGB_DISPLAY_NUM,
        .lock = NULL, /* 独占 SPI3，暂无互斥需求 */
    };
    configASSERT(BSP_STAT_TRUE == ws2812_led_init(&s_ws2812_led));

    /* TODO(灯效引擎接管)：首灯发红做上板验证——全黑帧无法区分"成功"和"没工作" */
    s_led_rgb_frame[0].r = 0xF0;

    printf("[WS2812] init OK, %d leds, period %dms\r\n", LED_RGB_DISPLAY_NUM, LED_RGB_DISPLAY_PERIOD_MS);

    uint32_t print_tick = 0;
    while (1)
    {
        /* TODO(灯效引擎接管)：改为 led_effect_tick(s_led_rgb_frame) 生成下一帧 +
           on-change 判断（帧没变就跳过发送，静态场景总线占用归零） */
        bsp_status_e status = ws2812_led_write(&s_ws2812_led, s_led_rgb_frame);

        print_tick += LED_RGB_DISPLAY_PERIOD_MS;
        if (print_tick >= LED_RGB_DISPLAY_PRINT_INTERVAL)
        {
            print_tick = 0;
            if (status != BSP_STAT_TRUE)
            {
                printf("[WS2812] write err: %d\r\n", status);
            }
        }

        vTaskDelayUntil(&PreviousWakeTime, pdMS_TO_TICKS(LED_RGB_DISPLAY_PERIOD_MS));
    }
}
