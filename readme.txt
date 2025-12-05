led_blink/
├── CMakeLists.txt                  # 顶层 CMake
├── cmake/
│   ├── gcc-arm-none-eabi.cmake     # 工具链配置
│   └── stm32f4xx_flags.cmake       # MCU 专用标志
├── Core/                           # 系统文件
│   ├── Inc/
│   ├── Src/
│   └── CMakeLists.txt
├── Drivers/                        # ST 标准外设库
│   ├── inc/
│   ├── src/
│   └── CMakeLists.txt
└── STM32F405RGTx_FLASH.ld          # 链接脚本

Project_Root
├── CMakeLists.txt          // 主构建脚本
├── cmake                   // 存放cmake辅助脚本
│   ├── arm-none-eabi-toolchain.cmake // 交叉编译工具链配置
│   ├── target_stm32f4.cmake    // F4 专用配置 (包含源文件路径、宏定义)
│   └── target_gd32f1.cmake     // F1 专用配置
├── Drivers
│   ├── STM32F4xx_StdPeriph_Driver // ST官方SPL库 (直接解压放这里)
│   │   ├── inc
│   │   └── src
│   └── GD32F10x_StdPeriph_Driver  // GD官方SPL库
│       ├── inc
│       └── src
├── Target
│   ├── Common              // 公共接口 (board_api.h)
│   ├── STM32F4_Port        // F4 的启动文件(.s), 链接脚本(.ld), BSP实现
│   │   ├── startup_stm32f40_41xxx.s  // 注意：必须是 gcc 版本的启动文件
│   │   └── stm32f405.ld              // F405 链接脚本
│   └── GD32F1_Port         // F1 的启动文件(.s), 链接脚本(.ld), BSP实现
│       ├── startup_gd32f10x_hd.s     // gcc 版本
│       └── gd32f103.ld               // F103 链接脚本
├── App                 // 应用层代码 (跨平台，纯逻辑)
│   ├── keymap.c        // 键位映射
│   ├── gui_app.c       // LVGL UI逻辑
│   └── lighting.c      // 灯效算法
├── Middlewares         // 第三方中间件
│   ├── FreeRTOS
│   ├── lvgl
│   └── CherryUSB       // 强烈推荐！比ST官方库更适合跨芯片移植
└── Utilities           // 常用工具 (RingBuffer, Log等)

MyKeyboard_Project/
├── CMakeLists.txt                  // [核心] 主构建脚本
├── cmake/                          // CMake 辅助脚本文件夹
│   ├── target_stm32f4.cmake        // [核心] F4 专用配置
│   └── target_gd32f1.cmake         // [核心] GD 专用配置
│
├── App/                            // === 应用层 (跨平台) ===
│   └── main.c                      // 业务逻辑：只调用 Board_API，不碰寄存器
│
├── Middlewares/                    // 中间件
│   ├── FreeRTOS/                   // (暂时留空)
│   └── lvgl/                       // (暂时留空)
│
├── Bsp/                         // === 适配层 (关键解耦层) ===
│   ├── Common/
│   │   └── board_api.h             // [接口] 定义 Board_Init, Board_LED_Toggle
│   │
│   ├── STM32F4_Port/               // F4 具体实现
│   │   ├── bsp_stm32f4.c           // [代码] 实现 board_api.h 里的接口
│   │   ├── stm32f405.ld            // [文件] 链接脚本 (从CubeMX或网上下)
│   │   └── startup_stm32f40_41xxx.s // [文件] 启动文件 (从库里复制出来)
│   │
│   └── GD32F1_Port/                // GD 具体实现
│       ├── bsp_gd32f1.c            // [代码] 实现 board_api.h 里的接口
│       ├── gd32f103.ld             // [文件] 链接脚本
│       └── startup_gd32f10x_hd.s   // [文件] 启动文件
│
└── Drivers/                        // === 原厂库文件 (物理隔离) ===
    ├── STM32F4_SPL/                // (STM32F4 标准库根目录)
    │   ├── STM32F4xx_StdPeriph_Driver/
    │   │   ├── inc/
    │   │   └── src/
    │   └── CMSIS/                  // <--- F4 CMSIS 在这里 !!
    │       ├── Include/            // (core_cm4.h 等)
    │       └── Device/ST/STM32F4xx/
    │           ├── Include/        // (stm32f4xx.h 等)
    │           └── Source/Templates/
    │               └── system_stm32f4xx.c // (必须被CMake引用)
    │
    └── GD32F1_SPL/                 // (GD32F1 标准库根目录)
        ├── GD32F10x_StdPeriph_Driver/
        │   ├── inc/
        │   └── src/
        └── CMSIS/                  // <--- GD CMSIS 在这里 !!
            ├── CM3/CoreSupport/    // (core_cm3.h 等)
            └── CM3/DeviceSupport/GD/GD32F10x/
                ├── Include/        // (gd32f10x.h 等)
                └── Source/
                    └── system_gd32f10x.c // (必须被CMake引用)