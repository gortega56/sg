cmake_minimum_required(VERSION 3.20)

project(sg VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

add_library(sg STATIC)

target_sources(sg 
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sg_allocator.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sg_assert.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sg_buffer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sg_hash_table.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sg_slice.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sg_vector.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/sg_types.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/sg_assert.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/sg_allocator.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/sg_buffer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/sg_hash_table.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/sg_slice.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/sg_vector.h"
)

target_include_directories(sg
	PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include)

set(SG_UNIT_TESTS_ENABLE OFF CACHE BOOL "Builds unit tests using gtest")
if (SG_UNIT_TESTS_ENABLE)
    enable_testing()
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/unit_tests)
endif()