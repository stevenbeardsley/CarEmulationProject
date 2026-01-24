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
