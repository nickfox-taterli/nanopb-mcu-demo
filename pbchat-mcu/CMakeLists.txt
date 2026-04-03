set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
cmake_minimum_required(VERSION 3.25)

# specify cross-compilers and tools
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER  arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# project settings
project(firmware C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# Cortex-M0+ - no FPU
add_compile_options(-mcpu=cortex-m0plus -mthumb -mthumb-interwork)
add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)

# Enable assembler files preprocessing
add_compile_options($<$<COMPILE_LANGUAGE:ASM>:-x$<SEMICOLON>assembler-with-cpp>)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_options(-Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_options(-Ofast -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    add_compile_options(-Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_options(-O0 -g)
endif ()

add_definitions(-DUSE_FULL_LL_DRIVER
        -DSTM32C092xx
        -DHSE_VALUE=8000000
        -DHSE_STARTUP_TIMEOUT=100
        -DLSE_STARTUP_TIMEOUT=5000
        -DLSE_VALUE=32768
        -DEXTERNAL_CLOCK_VALUE=12288000
        -DHSI_VALUE=48000000
        -DLSI_VALUE=32000
        -DVDD_VALUE=3300
        -DPREFETCH_ENABLE=0
        -DINSTRUCTION_CACHE_ENABLE=1
        -DDATA_CACHE_ENABLE=1
)

include_directories(
        Core/Inc
        App/Drivers
        App/Proto
        App/Proto/generated
        ThirdParty/nanopb
        Drivers/STM32C0xx_HAL_Driver/Inc
        Drivers/CMSIS/Device/ST/STM32C0xx/Include
        Drivers/CMSIS/Include
)

# Core - application sources
set(APP_SOURCES
        Core/Src/main.c
        Core/Src/board.c
        Core/Src/stm32c0xx_it.c
        Core/Src/syscalls.c
        Core/Src/sysmem.c
        Core/Src/system_stm32c0xx.c
)

# App-level drivers
set(APP_DRIVER_SOURCES
        App/Drivers/uart_driver.c
        App/Proto/proto_link.c
        App/Proto/generated/chat.pb.c
        ThirdParty/nanopb/pb_common.c
        ThirdParty/nanopb/pb_decode.c
        ThirdParty/nanopb/pb_encode.c
)

# Drivers - ST LL library
set(DRIVER_SOURCES
        Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_ll_dma.c
        Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_ll_exti.c
        Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_ll_gpio.c
        Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_ll_pwr.c
        Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_ll_rcc.c
        Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_ll_usart.c
        Drivers/STM32C0xx_HAL_Driver/Src/stm32c0xx_ll_utils.c
)

set(SOURCES
        ${APP_SOURCES}
        ${APP_DRIVER_SOURCES}
        ${DRIVER_SOURCES}
        ${CMAKE_SOURCE_DIR}/startup_stm32c092xx.s
)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/STM32C092XX_FLASH.ld)

add_link_options(-Wl,-gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map)
add_link_options(-mcpu=cortex-m0plus -mthumb -mthumb-interwork)
add_link_options(-T ${LINKER_SCRIPT})

add_executable(${PROJECT_NAME}.elf ${SOURCES} ${LINKER_SCRIPT})

target_link_libraries(${PROJECT_NAME}.elf PRIVATE)

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}
Building ${BIN_FILE}")
