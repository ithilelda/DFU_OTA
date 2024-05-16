cmake_minimum_required(VERSION 3.20)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# 编译器设置
add_compile_options(-march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore)
add_compile_options(
  -Werror
  -Wunused
  -fmessage-length=0
  -ffunction-sections
  -fdata-sections
  -fsigned-char
  -fno-common)

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
add_link_options(-T ${CMAKE_SOURCE_DIR}/link.ld)

# 设置项目名称
project(DFU_OTA LANGUAGES C CXX ASM)

# 编译目标设置
file(TO_CMAKE_PATH "$ENV{CH57X_DEPENDENCY}" DEPENDENCY)
file(TO_CMAKE_PATH "$ENV{LIBECC_DIR}" LIBECC)
add_executable(${PROJECT_NAME}.elf)
target_include_directories(${PROJECT_NAME}.elf PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${DEPENDENCY}/RVMSIS
  ${DEPENDENCY}/StdPeriphDriver/inc
  ${DEPENDENCY}/HAL/include
  ${DEPENDENCY}/LIB
  ${LIBECC}/include
)
target_link_directories(${PROJECT_NAME}.elf PRIVATE
  ${DEPENDENCY}/StdPeriphDriver
  ${DEPENDENCY}/LIB
)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/*.c
  ${DEPENDENCY}/RVMSIS/*.c
  ${DEPENDENCY}/StdPeriphDriver/*.c
  ${DEPENDENCY}/HAL/*.c
  ${LIBECC}/src/fp/*.c
  ${LIBECC}/src/nn/*.c
  ${LIBECC}/src/utils/*.c
  ${LIBECC}/src/hash/*.c
  ${LIBECC}/src/curves/*.c
  ${LIBECC}/src/ecdh/*.c
  ${LIBECC}/src/sig/*.c
)
target_sources(${PROJECT_NAME}.elf PRIVATE
  ${SOURCES}
  Startup/startup_ch573.S
)
target_link_libraries(${PROJECT_NAME}.elf ISP573 CH57xBLE)

# 用户宏定义
add_definitions(-DWITH_STDLIB)

add_definitions(-DCH57xBLE_ROM=1) # BLE库分开编译
add_definitions(-DBLE_BUFF_MAX_LEN=251) # 设置MTU最大值
add_definitions(-DBOOTLOADER_VERSION=1) # 设置bootloader版本
add_definitions(-DSIGNATURE_ED25519) # 设置签名算法

#后处理文件设置
set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(
  TARGET ${PROJECT_NAME}.elf
  POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
  COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
)

