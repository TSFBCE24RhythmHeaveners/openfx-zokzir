cmake_minimum_required(VERSION 3.10)

# Project name
project(Invert)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Source files
set(SOURCES invert.cpp)

# Add the library
add_library(Invert SHARED ${SOURCES})

# Link against OpenFX library
find_package(unofficial-openfx CONFIG REQUIRED)

# Link the OpenFX library to the target
target_link_libraries(Invert PRIVATE unofficial::openfx::OpenFx)

# Set the output name
set_target_properties(Invert PROPERTIES OUTPUT_NAME "invert" SUFFIX ".ofx")

# Set platform-specific output directories
if (WIN32)
    set_target_properties(Invert PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/invert.ofx.bundle/Contents/Win64")
elseif (APPLE)
    set_target_properties(Invert PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/invert.ofx.bundle/Contents/MacOS")
elseif (UNIX)
    set_target_properties(Invert PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/invert.ofx.bundle/Contents/Linux-x86-64")
endif()