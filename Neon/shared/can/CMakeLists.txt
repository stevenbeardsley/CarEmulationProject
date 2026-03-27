find_package(Boost REQUIRED COMPONENTS system)

set(can_message_HEADERS
    messages/ErrorMessage.h
    messages/CommandMessage.h
    messages/ICanMessage.h
    messages/StatusMessage.h
    messages/SpeedControl.h
)

set(can_message_SOURCE
    messages/ErrorMessage.cpp
    messages/CommandMessage.cpp
    messages/StatusMessage.cpp
    messages/SpeedControl.cpp
)

set(can_headers_HEADERS
    headers/Control.h
    headers/Error.h
    headers/Status.h
)

set(can_HEADERS
    Bus.h
    MessageCategory.h
    Receiver.h
)

set(can_SOURCE
    Bus.cpp
    Receiver.cpp
)

add_library(Can STATIC 
    ${can_SOURCE}
    ${can_HEADERS}
    ${can_headers_HEADERS}
    ${can_message_HEADERS}
    ${can_message_SOURCE}
)

target_include_directories(Can PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Threads is commonly needed on Linux with Boost.System / Asio setups
find_package(Threads REQUIRED)

target_link_libraries(Can
    PUBLIC 
        Boost::system
        Threads::Threads
    PRIVATE 
        BitParser
        LogFile
        shared
)

source_group("Source Files" FILES ${can_SOURCE})
source_group("Header Files" FILES ${can_HEADERS})

source_group("Header Files\\headers" FILES ${can_headers_HEADERS})
source_group("Header Files\\message" FILES ${can_message_HEADERS})
source_group("Source Files\\message" FILES ${can_message_SOURCE})

set_target_properties(Can PROPERTIES FOLDER "shared/can")