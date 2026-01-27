find_package(Boost REQUIRED COMPONENTS system)

set(can_HEADERS
    Bus.h
    MessageType.h
    Message.h
    Receiver.h
)

set(can_SOURCE
    Bus.cpp
    Message.cpp
    Receiver.cpp
)

add_library(Can STATIC 
    ${can_SOURCE}
    ${can_HEADERS}
)

target_include_directories(Can PUBLIC
    ${PROJECT_SOURCE_DIR}
)

# --- Boost.Asio deps ---
find_package(Boost REQUIRED COMPONENTS system)

# Threads is commonly needed on Linux with Boost.System / Asio setups
find_package(Threads REQUIRED)

target_link_libraries(Can
    PUBLIC 
        Boost::system
        Threads::Threads
    PRIVATE 
        BitParser
        LogFile
)
set_target_properties(Can PROPERTIES FOLDER "shared/can")
