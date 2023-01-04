# CMake 2.25 required; may change
cmake_minimum_required(VERSION 2.25)

# Needed for VERSION
cmake_policy(SET CMP0048 NEW)

# Set the project with latest version
project(shift VERSION 0.1.128)

# Set where exe files are produced
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set where dll files are produced
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set where archives files are produced
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# C++ settings

# This project requires c++17
set(CMAKE_CXX_FLAGS "-std=c++17 -Wall")
set(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

# List of sources for the C++ project
set(SOURCES src/main.cpp)

add_executable(shift ${SOURCES})