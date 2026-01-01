set(logFile_HEADERS
    LogFile.h
)

set(logFile_SOURCES
    LogFile.cpp
)

add_library(LogFile
    ${logFile_SOURCES}
    ${logFile_HEADERS}
)

target_include_directories(LogFile PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

