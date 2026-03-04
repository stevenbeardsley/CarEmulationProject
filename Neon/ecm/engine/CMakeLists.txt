set(engine_HEADERS
    Engine.h
)

set(engine_SOURCE
    Engine.cpp
)

set(engine_models_HEADERS
    models/FuelTank.h
    models/ThermalModel.h
)

set(engine_models_SOURCE
    models/FuelTank.cpp
    models/ThermalModel.cpp
)

add_library(Engine 
    ${engine_SOURCE}
    ${engine_HEADERS}
    ${engine_models_SOURCE}
    ${engine_models_HEADERS}
)

target_link_libraries(
    Engine
    PUBLIC 
        Can
        Config
        LogFile
)

target_include_directories(Engine PUBLIC
    ${PROJECT_SOURCE_DIR}
)

source_group("Source Files" FILES ${engine_SOURCE})
source_group("Header Files" FILES ${engine_HEADERS})

source_group("Source Files\\models" FILES ${engine_models_SOURCE})
source_group("Header Files\\models" FILES ${engine_models_HEADERS})

set_target_properties(Engine PROPERTIES FOLDER "components/ecm")
