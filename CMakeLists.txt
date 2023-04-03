cmake_minimum_required(VERSION 3.20)

project(sg VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

add_library(${PROJECT_NAME} STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/sg_vector.c)

target_include_directories( ${PROJECT_NAME} SYSTEM INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include">
    "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>")

source_group(include REGULAR_EXPRESSION ${CMAKE_CURRENT_SOURCE_DIR}/include/*.*)
source_group(src REGULAR_EXPRESSION ${CMAKE_CURRENT_SOURCE_DIR}/src/*.*)

enable_testing()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/unit_tests)