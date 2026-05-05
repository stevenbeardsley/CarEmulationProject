set(tcm_HEADERS
    Process.h
)

set(tcm_SOURCE
    Main.cpp
    Process.cpp
)

add_executable(Tcm
    ${tcm_SOURCE}
    ${tcm_HEADERS}
)

target_link_libraries(
    Tcm
    PUBLIC 
        LogFile
        Can
        shared
        Transmission
)
target_link_options(Tcm PRIVATE --coverage) 

target_include_directories(Tcm PUBLIC
    ${PROJECT_SOURCE_DIR}
)

source_group("Source Files" FILES ${tcm_SOURCE})
source_group("Header Files" FILES ${tcm_HEADERS})

set_target_properties(Tcm PROPERTIES FOLDER "components/tcm")
