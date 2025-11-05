


add_executable(EngineSim
	Main.cpp)

	target_link_libraries(EngineSim 
    Boost::system 
    Boost::json
    pthread  # Often needed for asio
)

target_include_directories(
    EngineSim
    PRIVATE ${Boost_INCLUDE_DIRS})