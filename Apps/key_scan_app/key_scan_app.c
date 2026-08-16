/**
 ******************************************************************************
 * @file    key_scan_app.c
 * @author  Pan
 * @brief   按键扫描任务（3片74HC165级联，10ms周期DMA读取）
 ******************************************************************************
 */

#include "key_scan_app.h"
#include "74hc165.h"
#include "FreeRTOS.h"
#include "bsp_interface.h"
#include "semphr.h"
#include "task.h"
#include <stdint.h>
#include <stdio.h>

#define KEY_SCAN_PERIOD_MS 10
#define KEY_SCAN_NUM_CHIPS 3
#define KEY_PRINT_INTERVAL 200 /* 打印间隔 ms，避免刷屏 */

/* ===== 按键 active level 配置（编译时位图） =====
 * 每片一字节，bit n = 该片第 n 键的 active level：
 *   1 = active-high（按下时输入电平 1）
 *   0 = active-low （按下时输入电平 0；按键串 GND + 上拉）
 * 当前接线：chip0/chip2 active-low，chip1 接法相反（active-high）*/
#define KEY_ACTIVE_LEVEL_BITMAP {0x00u, 0xFFu, 0x00u}

static hc165_t s_hc165;
/* DMA 接收缓冲区：必须是文件级 static（落 .bss / 主 SRAM），不可用任务局部数组
 * ——任务栈由 pvPortMalloc 分配在 CCMRAM，DMA 无法访问，会读到 0。*/
static uint8_t s_key_data[KEY_SCAN_NUM_CHIPS];
/* 各键 active level 位图（进 .rodata，按上面宏配置）*/
static const uint8_t s_key_active_level[KEY_SCAN_NUM_CHIPS] = KEY_ACTIVE_LEVEL_BITMAP;

/* DMA 同步（FreeRTOS 信号量注入 Component） */
static SemaphoreHandle_t s_spi_dma_sem;
static spi_dma_sync_t s_spi_dma_sync;

static bool spi_dma_wait(void* handle, uint32_t timeout_ms)
{
    return xSemaphoreTake((SemaphoreHandle_t)handle, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

static void spi_dma_notify_from_isr(void* handle)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR((SemaphoreHandle_t)handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief hc165 原始读数 → 按键状态（bit=1 表示按下）
 *
 * 1) Q7\ 错接补偿（临时，板子改 Q7→SER 后删本步）：硬件把上一片 Q7\(反相)
 *    接到了下一片 SER，导致级联链上距 MISO 奇数级(buf 奇数索引)的数据被
 *    整体取反一次，偶数级还原。奇数索引取反即可还原真实并行数据。
 * 2) 逐键 active level 归一化（XOR 一条算式搞定 8 键）：
 *      key = corrected XOR ~active_level
 *    active_level bit=0(low)  → ~0=1 → corrected^1 = ~corrected（低电平=按下→取反）
 *    active_level bit=1(high) → ~1=0 → corrected^0 = corrected（高电平=按下→原值）
 */
static void hc165_raw_to_keys(const uint8_t raw[KEY_SCAN_NUM_CHIPS], uint8_t keys[KEY_SCAN_NUM_CHIPS])
{
    for (uint8_t i = 0; i < KEY_SCAN_NUM_CHIPS; i++)
    {
        uint8_t corrected = (i & 1U) ? (uint8_t)~raw[i] : raw[i]; /* 1) Q7\ 补偿 */
        keys[i] = corrected ^ (uint8_t)~s_key_active_level[i];    /* 2) 逐键归一化 */
    }
}

void Key_Scan_Task(void* param)
{
    static TickType_t PreviousWakeTime;
    PreviousWakeTime = xTaskGetTickCount();

    /* 创建 DMA 同步信号量 */
    s_spi_dma_sem = xSemaphoreCreateBinary();
    configASSERT(s_spi_dma_sem != NULL);
    s_spi_dma_sync.handle = s_spi_dma_sem;
    s_spi_dma_sync.wait = spi_dma_wait;
    s_spi_dma_sync.notify_from_isr = spi_dma_notify_from_isr;

    /* 初始化 74HC165：预填描述符（指定初始化器，字段自文档），再校验+硬件 init */
    s_hc165 = (hc165_t){
        .spi_ops = g_board_hw_bsp_->spi_ops,
        .spi_id = SPI_ID_KEY_SCAN,
        .dma_sync = &s_spi_dma_sync, /* NULL = 轮询模式 */
        .num_chips = KEY_SCAN_NUM_CHIPS,
        .gpio_ops = g_board_hw_bsp_->gpio_ops,
        .pl_pin = GPIO_PIN_HC165_PL,
        .lock = NULL, /* 独占 SPI2，暂无互斥需求 */
    };
    configASSERT(BSP_STAT_TRUE == hc165_init(&s_hc165));

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
                uint8_t keys[KEY_SCAN_NUM_CHIPS];
                // TODO:
                hc165_raw_to_keys(s_key_data, keys);
                /* 调试：raw(补偿前) vs key(补偿+归一化后)；验证 OK 后可只留 key */
                printf("[KEY] raw:%02X %02X %02X | key:%02X %02X %02X\r\n", s_key_data[0], s_key_data[1], s_key_data[2],
                       keys[0], keys[1], keys[2]);
            }
            else
            {
                printf("[HC165] read err: %d\r\n", status);
            }
        }

        vTaskDelayUntil(&PreviousWakeTime, pdMS_TO_TICKS(KEY_SCAN_PERIOD_MS));
    }
}
