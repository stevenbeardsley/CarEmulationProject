set(bit_parser_HEADERS
    ByteBuffer.h
    BitWriter.h
    BitReader.h
    Error.h
    BitOrder.h
    ByteOrder.h
    Span.h
)
set(bit_parser_SOURCE
    BitParser.cpp
)


add_library(BitParser STATIC
    ${bit_parser_HEADERS}
    ${bit_parser_SOURCE}
)

target_include_directories(BitParser
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
)

source_group("Header Files" FILES ${bit_parser_HEADERS})

set_target_properties(BitParser PROPERTIES FOLDER "shared")