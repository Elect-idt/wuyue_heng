/**
 ******************************************************************************
 * @file    key_scan_app.c
 * @author  Pan
 * @brief   按键扫描任务（3片74HC165级联，10ms周期DMA读取）
 ******************************************************************************
 */

#include "key_scan_app.h"
#include "74hc165.h"
#include "semphr.h"
#include <stdint.h>
#include <stdio.h>

#define KEY_SCAN_PERIOD_MS 10
#define KEY_SCAN_NUM_CHIPS 3
#define KEY_SCAN_TASK_PRI 4
#define KEY_PRINT_INTERVAL 200 /* 打印间隔 ms，避免刷屏 */

static hc165_t s_hc165;
static uint8_t s_key_data[KEY_SCAN_NUM_CHIPS];

/* DMA 同步（FreeRTOS 信号量注入 Component） */
static SemaphoreHandle_t s_spi_dma_sem;
static spi_dma_sync_t s_spi_dma_sync;

static void spi_dma_wait(void* handle) { xSemaphoreTake((SemaphoreHandle_t)handle, portMAX_DELAY); }

static void spi_dma_notify_from_isr(void* handle)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR((SemaphoreHandle_t)handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void Key_Scan_Task(void* param)
{
    static portTickType PreviousWakeTime;
    PreviousWakeTime = xTaskGetTickCount();

    /* 创建 DMA 同步信号量 */
    s_spi_dma_sem = xSemaphoreCreateBinary();
    s_spi_dma_sync.handle = s_spi_dma_sem;
    s_spi_dma_sync.wait = spi_dma_wait;
    s_spi_dma_sync.notify_from_isr = spi_dma_notify_from_isr;

    /* 初始化 74HC165（3 片级联） */
    hc165_init(&s_hc165, g_board_hw_bsp_->spi_ops, SPI_ID_KEY_SACN, &s_spi_dma_sync, KEY_SCAN_NUM_CHIPS,
               g_board_hw_bsp_->gpio_ops, GPIO_PIN_HC165_PL);

    printf("[HC165] init OK, %d chips, period %dms\r\n", KEY_SCAN_NUM_CHIPS, KEY_SCAN_PERIOD_MS);

    uint32_t print_tick = 0;
    while (1)
    {
        bsp_status_e status = hc165_read(&s_hc165, s_key_data);

        print_tick += KEY_SCAN_PERIOD_MS;
        if (print_tick >= KEY_PRINT_INTERVAL)
        {
            print_tick = 0;
            if (status == BSP_STAT_TRUE)
            {
                printf("[HC165] %02X %02X %02X\r\n", s_key_data[0], s_key_data[1], s_key_data[2]);
            }
            else
            {
                printf("[HC165] read err: %d\r\n", status);
            }
        }

        vTaskDelayUntil(&PreviousWakeTime, pdMS_TO_TICKS(KEY_SCAN_PERIOD_MS));
    }
}
