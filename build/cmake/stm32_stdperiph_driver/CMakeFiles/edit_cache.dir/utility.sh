set -e

cd /j/ArmsApprentice/wuyue_heng/0.reference/led_blink/build/cmake/stm32_stdperiph_driver
/usr/bin/ccmake.exe -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
