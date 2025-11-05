cmake_minimum_required(VERSION 3.20)
project(NeonProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Boost once globally (vcpkg will handle include and libs)
find_package(Boost 1.75 REQUIRED COMPONENTS system json)

# Make Boost available to all subdirectories
# This exports the variables Boost_INCLUDE_DIRS, Boost_LIBRARIES, etc.
include_directories(${Boost_INCLUDE_DIRS})

# Add subdirectories for the executables
add_subdirectory(ecm)
add_subdirectory(tcm)
add_subdirectory(dashboard)
