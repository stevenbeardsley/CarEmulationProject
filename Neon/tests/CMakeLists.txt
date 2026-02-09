include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# Windows: avoid CRT mismatch issues
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

set(test_bit_parser_SOURCE
    TestBitParser.cpp
)
add_executable(BitParserTests 
    ${test_bit_parser_SOURCE}
)

target_include_directories(BitParserTests PUBLIC
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(BitParserTests
    PRIVATE 
        gtest
        gtest_main
        BitParser
)

set_target_properties(BitParserTests PROPERTIES FOLDER "tests")

# CAN TESTS 
set(test_can_SOURCE
    TestCan.cpp
)

add_executable(CanTests 
    ${test_can_SOURCE}
)

target_include_directories(CanTests PUBLIC
    ${PROJECT_SOURCE_DIR}
)
target_link_libraries(CanTests
    PRIVATE 
        gtest
        gtest_main
        Can
)

set_target_properties(CanTests PROPERTIES FOLDER "tests")

# Config tests
set(test_config_SOURCE
    TestConfig.cpp
)

add_executable(ConfigTests 
    ${test_config_SOURCE}
)

target_include_directories(ConfigTests PUBLIC
    ${PROJECT_SOURCE_DIR}
)
target_link_libraries(ConfigTests
    PRIVATE 
        gtest
        gtest_main
        Config
)

set_target_properties(ConfigTests PROPERTIES FOLDER "tests")