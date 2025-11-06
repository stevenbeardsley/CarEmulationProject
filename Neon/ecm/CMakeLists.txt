add_executable(Ecm
	Main.cpp)

target_include_directories(
    Ecm
    PRIVATE ${Boost_INCLUDE_DIRS})