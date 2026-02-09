set(config_HEADERS
    Config.h
    Engine.h
    Transmission.h
    VehicleConfig.h
)

set(config_SOURCES
    Config.cpp
)

add_library(Config STATIC 
    ${config_SOURCES}
    ${config_HEADERS}
)

target_link_libraries(Config
    PUBLIC
    nlohmann_json::nlohmann_json
)

target_include_directories(Config PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
source_group("Source Files" FILES ${config_SOURCE})
source_group("Header Files" FILES ${config_HEADERS})
set_target_properties(Config PROPERTIES FOLDER "shared/config")
