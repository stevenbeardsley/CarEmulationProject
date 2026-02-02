set(engine_HEADERS
    Engine.h
)

set(engine_SOURCE
    Engine.cpp    
)

add_library(Engine 
    ${engine_SOURCE}
    ${engine_HEADERS}
)

target_link_libraries(
    Engine
    PUBLIC 
        LogFile
        Can
)

source_group("Source Files" FILES ${engine_SOURCE})
source_group("Header Files" FILES ${engine_HEADERS})

set_target_properties(Engine PROPERTIES FOLDER "components/ecm")
