# CMake 3.23 required to build
cmake_minimum_required(VERSION 3.23.0)

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
set(
    SOURCES 
    src/main.cpp 
    src/compiler/shift_argument_parser.cpp 
    src/compiler/shift_error_handler.cpp
    src/compiler/shift_parser.cpp 
    src/compiler/shift_tokenizer.cpp 
    src/filesystem/directory.cpp 
    src/filesystem/drive.cpp 
    src/filesystem/file.cpp
    src/logging/console.cpp 
    src/utils/utils.cpp
    )

# List of include directores for the project
set(INCLUDES include src)

# List of library directores for the project
set(LIBRARY_DIRECTORIES lib)

# List of libraries the project utilizes
set(LIBRARIES "")

# Set the executable file for the project
add_executable(shift ${SOURCES})

# shift can only compile with c++17 and later
target_compile_features(shift PRIVATE cxx_std_17)

# Add includes for the project
target_include_directories(shift PRIVATE ${INCLUDES})

# Add library directories for the project
target_link_directories(shift PRIVATE ${LIBRARY_DIRECTORIES})

# Add and link libraries to project
target_link_libraries(shift ${LIBRARIES})

# Most of the shift compiler code will be implemented inside DLL files, while the executable compiler itself will simply link and call to those DLLs
# Planning for this functionality, it is required to specify a build type for shift (static or dynamic (dll))
# For now, this affects nothing since shift is being entirely built into the executable.
target_compile_definitions(shift PRIVATE "SHIFT_BUILD_STATIC=1")

# Enable lto on project if supported
set_property(TARGET shift PROPERTY INTERPROCEDURAL_OPTIMIZATION True)