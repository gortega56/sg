cmake_minimum_required(VERSION 3.20)

project(sg VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

add_library(${PROJECT_NAME} STATIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/sg_types.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/sg_assert.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/sg_allocator.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/sg_buffer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/sg_hash_table.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/sg_slice.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/sg_vector.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sg_allocator.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sg_assert.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sg_buffer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sg_hash_table.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sg_slice.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sg_vector.c"
)

target_include_directories( ${PROJECT_NAME} INTERFACE PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>")

enable_testing()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/unit_tests)