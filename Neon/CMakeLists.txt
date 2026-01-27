cmake_minimum_required(VERSION 3.20)
project(NeonProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost CONFIG REQUIRED COMPONENTS system json)

add_subdirectory(ecm)
add_subdirectory(tcm)
add_subdirectory(dashboard)
add_subdirectory(shared/logging)
add_subdirectory(shared/bit_parser)
add_subdirectory(shared/can)
add_subdirectory(shared)

add_subdirectory(tests)

# Enables IDE folder grouping (Visual Studio, Xcode, etc.)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)