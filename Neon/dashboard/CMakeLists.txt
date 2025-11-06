set(dashboard_SOURCES Main.cpp)

add_executable(Dashboard ${dashboard_SOURCES})

target_link_libraries(Dashboard
    PRIVATE
        Boost::system
        Boost::json
)

source_group("Source Files" FILES ${dashboard_SOURCES})
