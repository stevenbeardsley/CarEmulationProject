set(transmission_HEADERS
    Transmission.h
)

set(transmission_SOURCE
    Transmission.cpp    
)

add_library(Transmission 
    ${transmission_SOURCE}
    ${transmission_HEADERS}
)

target_link_libraries(
    Transmission
    PUBLIC 
        LogFile
        Config
)

source_group("Source Files" FILES ${transmission_SOURCE})
source_group("Header Files" FILES ${transmission_HEADERS})

set_target_properties(Transmission PROPERTIES FOLDER "components/tcm")
