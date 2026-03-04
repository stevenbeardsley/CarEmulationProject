find_package(Boost REQUIRED COMPONENTS system json)

set(dashboard_HEADERS
    CommandMessage.h  
    DashboardDataSource.h
    Process.h
    CommandHttpServer.h
    WebSocketReceiver.h
    UiData.h
)

set(dashboard_SOURCE
    Main.cpp
    DashboardDataSource.cpp
    Process.cpp
    CommandHttpServer.cpp
    WebSocketReceiver.cpp)

add_executable(Dashboard 
    ${dashboard_SOURCE}
    ${dashboard_HEADERS}
)

target_include_directories(Dashboard PUBLIC
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(
    Dashboard
    PUBLIC 
        LogFile
    PRIVATE
        BitParser
        Can
        Boost::system
        Boost::json
        Config
        shared
)


source_group("Source Files" FILES ${dashboard_SOURCE})
source_group("Header Files" FILES ${dashboard_HEADERS})

set_target_properties(Dashboard PROPERTIES FOLDER "components/dashboard")
