cmake_minimum_required(VERSION 3.20)
project(NeonProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost CONFIG REQUIRED COMPONENTS system json)

find_package(nlohmann_json CONFIG REQUIRED)
enable_testing()
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    link_libraries(ws2_32 mswsock)
endif()

find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY 
        "${CLANG_TIDY_EXE};--quiet;--header-filter=^${CMAKE_SOURCE_DIR}/.*;--checks=bugprone-*,modernize-*,performance-*"
    )
endif()

add_subdirectory(ecm) 
add_subdirectory(ecm/engine)
add_subdirectory(tcm)
add_subdirectory(tcm/transmission)

add_subdirectory(dashboard)

if(ENABLE_COVERAGE)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()
add_subdirectory(shared)
add_subdirectory(shared/bit_parser)
add_subdirectory(shared/can)
add_subdirectory(shared/config)
add_subdirectory(shared/logging)


add_subdirectory(tests)

# Enables IDE folder grouping (Visual Studio, Xcode, etc.)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)