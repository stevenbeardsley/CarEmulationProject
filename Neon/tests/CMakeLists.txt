include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# Windows: avoid CRT mismatch issues
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)
include(GoogleTest)

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

# Register with CTest
gtest_discover_tests(BitParserTests)


set(test_can_SOURCE
    TestBus.cpp
    TestErrorMessage.cpp
    TestGearChangeMessage.cpp
    TestStatusMessage.cpp
    TestThrottleMessage.cpp
    TestReceiver.cpp
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

# Register with CTest
gtest_discover_tests(CanTests)


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

# Register with CTest
gtest_discover_tests(ConfigTests)


# GCOV COVERAGE FLAGS
# If coverage is enabled, apply the necessary compiler and linker flags 
# to all test targets so they can properly interact with gcov.
if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # A list of all your test executables
    set(ALL_TEST_TARGETS BitParserTests CanTests ConfigTests)
    
    foreach(TEST_TARGET IN LISTS ALL_TEST_TARGETS)
        target_compile_options(${TEST_TARGET} PRIVATE --coverage -O0 -g)
        target_link_options(${TEST_TARGET} PRIVATE --coverage)
    endforeach()
endif()