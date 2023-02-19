# CMake 3.21 required to build
cmake_minimum_required(VERSION 3.21)

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
    src/compiler/shift_analyzer.cpp
    src/compiler/shift_argument_parser.cpp
    src/compiler/shift_compiler.cpp
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
add_executable(${PROJECT_NAME} ${SOURCES})

# shift can only compile with c++17 and later
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# Add includes for the project
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDES})

# Add library directories for the project
target_link_directories(${PROJECT_NAME} PRIVATE ${LIBRARY_DIRECTORIES})

# Add and link libraries to project
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

# Most of the shift compiler code will be implemented inside DLL files, while the executable compiler itself will simply link and call to those DLLs
# Planning for this functionality, it is required to specify a build type for shift (static or dynamic (dll))
# For now, this affects nothing since shift is being entirely built into the executable.
target_compile_definitions(shift PRIVATE "SHIFT_BUILD_STATIC=1")

# Enable lto on the target if supported (in Release mode)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
endif()

# Enable PIE/PIC on the target if supported
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE True)

# Allow installation of the target
install(TARGETS ${PROJECT_NAME})