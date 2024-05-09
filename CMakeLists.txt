cmake_minimum_required(VERSION 3.20)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_C_STANDARD 99)
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
project(ch57x_template LANGUAGES C CXX ASM)

# 编译目标设置
file(TO_CMAKE_PATH "$ENV{CH57X_DEPENDENCY}" DEPENDENCY)
add_executable(${PROJECT_NAME}.elf)
target_include_directories(${PROJECT_NAME}.elf PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${DEPENDENCY}/RVMSIS
  ${DEPENDENCY}/StdPeriphDriver/inc
  ${DEPENDENCY}/HAL/include
  ${DEPENDENCY}/LIB
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
)
target_sources(${PROJECT_NAME}.elf PRIVATE
  ${SOURCES}
  Startup/startup_ch573.S
)
target_link_libraries(${PROJECT_NAME}.elf ISP573 CH57xBLE)

# 用户宏定义
add_definitions(-DCH57xBLE_ROM=1)


#后处理文件设置
set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(
  TARGET ${PROJECT_NAME}.elf
  POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
  COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf>
          ${BIN_FILE})

