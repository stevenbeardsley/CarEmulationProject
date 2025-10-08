cmake_minimum_required(VERSION 3.20)
project(NeonProject)

set(CMAKE_CXX_STANDARD 17)

# Add subdirectories for the executables
add_subdirectory(ecm)          # Engine executable
add_subdirectory(tcm)    # Transmission executable
