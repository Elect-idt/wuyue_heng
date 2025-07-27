set -e

cd /j/ArmsApprentice/wuyue_heng/0.reference/led_blink/build
/usr/bin/ccmake.exe -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
