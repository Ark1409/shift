# CMake 2.23 required; may change
cmake_minimum_required(VERSION 2.23)

# Needed for project VERSION
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

# List of sources for the project
set(SOURCES src/main.cpp)

# List of include directores for the project
set(INCLUDES include)

# List of library directores for the project
set(LIBRARY_DIRECTORIES lib)

# List of libraries the project utilizes
set(LIBRARIES "")

# Set the executable file for the project
add_executable(shift ${SOURCES})

# Add includes for the project
target_include_directories(shift PRIVATE ${INCLUDES})

# Add library directories for the project
target_link_directories(shift PRIVATE ${LIBRARY_DIRECTORIES})

# Add and link libraries to project
target_link_libraries(shift ${LIBRARIES})