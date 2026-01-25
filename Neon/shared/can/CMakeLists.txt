set(can_HEADERS
    MessageType.h
    Message.h
)

set(can_SOURCE
    Message.cpp
)

add_library(Can STATIC 
    ${can_SOURCE}
    ${can_HEADERS}
)

target_include_directories(Can PUBLIC
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(Can
    PRIVATE 
        BitParser
)
set_target_properties(Can PROPERTIES FOLDER "shared")
