cmake_minimum_required(VERSION 3.20)

# Pull in Pico SDK (must be before project())
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(axisOS
    VERSION     1.4.0
    DESCRIPTION "AxisOS — Pico 2W Firmware"
    LANGUAGES   C CXX ASM
)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

# Initialise the SDK
pico_sdk_init()

# -----------------------------------------------------------------------
#  Compiler flags
# -----------------------------------------------------------------------
add_compile_options(
    -Wall
    -Wextra
    -Wno-unused-parameter
    -ffunction-sections
    -fdata-sections
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O2)
else()
    add_compile_options(-Og -g3)
    add_compile_definitions(AXISOS_DEBUG=1)
endif()

# -----------------------------------------------------------------------
#  Source files
# -----------------------------------------------------------------------
set(AXISOS_SOURCES
    src/main.c
    src/axisOS_core.c
    src/axisOS_wifi.c
    src/axisOS_hal.c
    src/axisOS_tasks.c
)

# -----------------------------------------------------------------------
#  Target
# -----------------------------------------------------------------------
add_executable(axisOS ${AXISOS_SOURCES})

target_include_directories(axisOS PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# -----------------------------------------------------------------------
#  SDK Libraries
# -----------------------------------------------------------------------
target_link_libraries(axisOS
    pico_stdlib
    pico_cyw43_arch_lwip_poll
    hardware_gpio
    hardware_adc
    hardware_i2c
    hardware_spi
    hardware_watchdog
    hardware_clocks
)

# -----------------------------------------------------------------------
#  USB / UART stdio
# -----------------------------------------------------------------------
pico_enable_stdio_usb(axisOS  1)
pico_enable_stdio_uart(axisOS 0)

# -----------------------------------------------------------------------
#  Generate UF2 + map + disassembly
# -----------------------------------------------------------------------
pico_add_extra_outputs(axisOS)

# -----------------------------------------------------------------------
#  Version stamp via compiler define
# -----------------------------------------------------------------------
target_compile_definitions(axisOS PRIVATE
    AXISOS_CMAKE_VERSION="${PROJECT_VERSION}"
    AXISOS_BUILD_DATE="${CMAKE_BUILD_DATE}"
)

# -----------------------------------------------------------------------
#  Post-build: copy UF2 to release/ directory
# -----------------------------------------------------------------------
add_custom_command(TARGET axisOS POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_SOURCE_DIR}/release
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/axisOS.uf2
            ${CMAKE_SOURCE_DIR}/release/axisOS-v${PROJECT_VERSION}.uf2
    COMMENT "Copying UF2 to release/"
)
