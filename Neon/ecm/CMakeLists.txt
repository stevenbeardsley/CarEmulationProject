set(ecm_HEADERS
    Process.h
)

set(ecm_SOURCE
    Main.cpp
    Process.cpp
)

set(ecm_can_HEADERS
    can/Subscriptions.h
)

add_executable(Ecm
    ${ecm_SOURCE}
    ${ecm_HEADERS}
    ${ecm_can_HEADERS}   
)

target_link_libraries(
    Ecm
    PUBLIC
        LogFile
        Can
        Engine
        shared
)

target_link_options(Ecm PRIVATE --coverage) 

target_include_directories(Ecm PUBLIC
    ${PROJECT_SOURCE_DIR}
)

source_group("Source Files" FILES ${ecm_SOURCE})
source_group("Header Files" FILES ${ecm_HEADERS})
source_group("Header Files\\can" FILES ${ecm_can_HEADERS}) 

set_target_properties(Ecm PROPERTIES FOLDER "components/ecm")
