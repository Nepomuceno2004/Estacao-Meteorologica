
cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)
project(Estacao-Meteorologica C CXX ASM)
pico_sdk_init()


include_directories( ${CMAKE_SOURCE_DIR}/lib ) # Inclui os files .h na pasta lib

add_executable(${PROJECT_NAME}  
        Estacao-Meteorologica.c 
        lib/aht20.c 
        lib/bmp280.c 
        lib/ssd1306.c
        lib/buzzer.c
        lib/matrizLed.c
        )

target_link_libraries(${PROJECT_NAME} 
        pico_stdlib 
        hardware_i2c
        hardware_pwm
        pico_cyw43_arch_lwip_threadsafe_background
        )

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})