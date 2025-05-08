cmake_minimum_required(VERSION 3.20)

project(stone)

#
# Environment
#
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 17)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(cmake/stone_functions.cmake)
include_directories(src)

#
# Packages
#
stone_build_package(stone)
stone_build_package(example)
