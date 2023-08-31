# CMake 3.21 required to build
cmake_minimum_required(VERSION 3.21)

# Needed for project VERSION
cmake_policy(SET CMP0048 NEW)

# Set the project with latest version
project(shift VERSION 0.1.128)

# Add option as to whether tests should be built (requires googletest download)
option(SHIFT_TEST "Build shift tests" ON)

# Set where exe files are produced
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set where dll files are produced
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set where archives files are produced
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# C++ settings

# Create the compiler library for the project
add_library(
    ${PROJECT_NAME} 
    SHARED
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

add_library("${PROJECT_NAME}::${PROJECT_NAME}" ALIAS ${PROJECT_NAME})

# shift can only compile with c++17 and later
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

# Add includes for the project
target_include_directories(${PROJECT_NAME} PUBLIC include src)

# Add library directories for the project
target_link_directories(${PROJECT_NAME} PUBLIC "")

# Add and link libraries to project
target_link_libraries(${PROJECT_NAME} PUBLIC "")

# Most of the shift compiler code will be implemented inside DLL files, while the executable compiler itself will simply link and call to those DLLs
# Planning for this functionality, it is required to specify a build type for shift (static or dynamic (dll))
# For now, this affects nothing since shift is being entirely built into the executable.
target_compile_definitions(${PROJECT_NAME} PRIVATE "SHIFT_BUILD=1")
target_compile_definitions(${PROJECT_NAME} PUBLIC "SHIFT_BUILD_DLL=1")

# Enable lto on the target if supported (in Release mode)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
endif()

# Enable PIE/PIC on the target if supported
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE True)

# Remove "lib" prefix on windows operating system
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
endif()

# Set the executable file (runner) for the project
add_executable(
    ${PROJECT_NAME}-runner 
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME}-runner PRIVATE ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME}-runner PROPERTIES INTERPROCEDURAL_OPTIMIZATION True)
set_target_properties(${PROJECT_NAME}-runner PROPERTIES POSITION_INDEPENDENT_CODE True)
set_target_properties(${PROJECT_NAME}-runner PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

# Download gtest if testing is enabled
if(SHIFT_TEST)
    # Fetch googletest library
    include(FetchContent)

    # Download gtest repository through GitHub
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG f8d7d77c06936315286eb55f8de22cd23c188571 # v1.14.0
        GIT_PROGRESS true)

    # Disable unwanted googletest options
    option(BUILD_GMOCK "Builds the googlemock subproject" OFF)
    option(INSTALL_GTEST "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)" OFF)
    option(GTEST_HAS_ABSL "Use Abseil and RE2. Requires Abseil and RE2 to be separately added to the build." OFF)

    # Populate project with googletest library
    FetchContent_MakeAvailable(googletest)

    # Enable ctest after ensuring googletest has been made available
    enable_testing()

    # Create test executable
    add_executable(
        ${PROJECT_NAME}-test
        test/src/argument_parser_test.cpp
        test/src/tokenizer_test.cpp
    )

    # Set test project properties and libraries
    set_target_properties(${PROJECT_NAME}-test PROPERTIES INTERPROCEDURAL_OPTIMIZATION True)
    set_target_properties(${PROJECT_NAME}-test PROPERTIES POSITION_INDEPENDENT_CODE True)
    target_link_libraries(${PROJECT_NAME}-test PRIVATE ${PROJECT_NAME} PRIVATE GTest::gtest PRIVATE GTest::gtest_main)

    # Register test executable with ctest
    add_test(
        NAME ${PROJECT_NAME}-test
        COMMAND ${PROJECT_NAME}-test
    )
endif()