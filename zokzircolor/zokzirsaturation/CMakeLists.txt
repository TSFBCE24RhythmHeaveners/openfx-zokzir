cmake_minimum_required(VERSION 3.10)

# Project name
project(ZokzirSaturation)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Source files
set(SOURCES zokzirsaturation.cpp)

# Add the library
add_library(Invert SHARED ${SOURCES})

# Link the OpenFX library to the target
target_link_libraries(ZokzirSaturation PRIVATE zokzir::openfx::OpenFx)

# Set the output name
set_target_properties(ZokzirSaturation PROPERTIES OUTPUT_NAME "ZokzirSaturation" SUFFIX ".ofx")

# Set platform-specific output directories
if (WIN32)
    set_target_properties(ZokzirSaturation PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/ZokzirSaturation.ofx.bundle/Contents/Win64")
elseif (APPLE)
    set_target_properties(ZokzirSaturation PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/ZokzirSaturation.ofx.bundle/Contents/MacOS")
elseif (UNIX)
    set_target_properties(ZokzirSaturation PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/ZokzirSaturation.ofx.bundle/Contents/Linux-x86-64")
endif()
