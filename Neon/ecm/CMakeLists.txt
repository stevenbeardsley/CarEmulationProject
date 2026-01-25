set(ecm_HEADERS
    Process.h
)

set(ecm_SOURCE
    Main.cpp
    Process.cpp
)

add_executable(Ecm
    ${ecm_SOURCE}
    ${ecm_HEADERS}
)

target_link_libraries(
    Ecm
    PUBLIC 
        LogFile
        Can
)

source_group("Source Files" FILES ${ecm_SOURCE})
source_group("Header Files" FILES ${ecm_HEADERS})

set_target_properties(Ecm PROPERTIES FOLDER "components/ecm")
