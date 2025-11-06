find_package(Boost REQUIRED COMPONENTS system json)
set(dashboard_HEADERS
    DashboardDataSource.h)

set(dashboard_SOURCE
    Main.cpp
    DashboardDataSource.cpp)

add_executable(Dashboard 
    ${dashboard_SOURCE}
    ${dashboard_HEADERS}
)

target_link_libraries(Dashboard
    PRIVATE
        Boost::system
        Boost::json
)

source_group("Source Files" FILES ${dashboard_SOURCE})
source_group("Header Files" FILES ${dashboard_HEADERS})
