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