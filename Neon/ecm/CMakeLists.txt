add_executable(Ecm
	Main.cpp)

target_include_directories(
    Ecm
    PRIVATE ${Boost_INCLUDE_DIRS})

set_target_properties(Ecm PROPERTIES FOLDER "components/ecm")
