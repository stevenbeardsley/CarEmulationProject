find_package(Boost REQUIRED COMPONENTS system json)

set(dashboard_HEADERS
    DashboardDataSource.h
    Process.h
    CommandWebSocketServer.h
    WebSocketReceiver.h)

set(dashboard_SOURCE
    Main.cpp
    DashboardDataSource.cpp
    Process.cpp
    CommandWebSocketServer.cpp
    WebSocketReceiver.cpp)

add_executable(Dashboard 
    ${dashboard_SOURCE}
    ${dashboard_HEADERS}
)

target_link_libraries(
    Dashboard
    PUBLIC 
        LogFile
    PRIVATE
        Boost::system
        Boost::json
)

source_group("Source Files" FILES ${dashboard_SOURCE})
source_group("Header Files" FILES ${dashboard_HEADERS})

set_target_properties(Dashboard PROPERTIES FOLDER "components/dashboard")
