add_executable(DashboardSim
    Main.cpp
)

# Link against Boost libraries already found by the parent
target_link_libraries(DashboardSim
    PRIVATE
        Boost::system
        Boost::json
)

# If you’re using Boost.Asio or Beast, you might need pthread (Linux only)
if(UNIX)
    target_link_libraries(DashboardSim PRIVATE pthread)
endif()

# (Optional, usually not necessary if parent added include_directories)
target_include_directories(DashboardSim PRIVATE ${Boost_INCLUDE_DIRS})
