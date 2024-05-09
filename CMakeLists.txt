
cmake_minimum_required(VERSION 3.20)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

# 设置项目名称
project(DFU_OTA LANGUAGES C CXX ASM)

# MCU设置
add_compile_options(-march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore)

# 编译设置
add_compile_options(
  -Werror
  -Wunused
  -fmessage-length=0
  -ffunction-sections
  -fdata-sections
  -fsigned-char
  -fno-common)

# 处理ASM文件
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
  add_compile_options(-Og -g)
endif ()


include_directories(
  "src"
  "../RVMSIS"
  "../StdPeriphDriver/inc"
  "../HAL/include"
  "../LIB")

link_directories(
  "../StdPeriphDriver"
  "../LIB"
)

file(GLOB_RECURSE SOURCES
  "src/*.c"
  "Startup/startup_ch573.S"
  "../RVMSIS/*.c"
  "../StdPeriphDriver/*.c"
  "../HAL/*.c")

# 用户宏定义
add_definitions(-DCH57xBLE_ROM=1)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/link.ld)
# 链接设置
add_link_options(
  -march=rv32imac
  -mabi=ilp32
  -nostartfiles
  -Xlinker
  --gc-sections
  -Wl,--print-memory-usage
  -Wl,-Map,${PROJECT_NAME}.map
  --specs=nano.specs
  --specs=nosys.specs)
add_link_options(-T ${LINKER_SCRIPT})

add_executable(${PROJECT_NAME}.elf ${SOURCES} ${LINKER_SCRIPT})
target_link_libraries(${PROJECT_NAME}.elf ISP573 CH57xBLE)

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(
  TARGET ${PROJECT_NAME}.elf
  POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
  COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf>
          ${BIN_FILE})

