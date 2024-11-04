# CMake 3.21 required to build
cmake_minimum_required(VERSION 3.21)

# Set the project name for tests
project(shift-test)

# Include FetchContent for repository cloning
include(FetchContent)

# Inform user of operation in case it takes longer than expected
message(STATUS "Fetching googletest library (this may take a minute)...")

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

# Inform user of operation completion
message(STATUS "Fetching googletest library completed.")

# Create test executable
add_executable(
        ${PROJECT_NAME}
        src/argument_parser_test.cpp
        src/tokenizer_test.cpp
        src/parser_test.cpp
)

# Set test project properties and libraries
set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION True)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE True)
target_link_libraries(${PROJECT_NAME} PRIVATE shift::shift PRIVATE GTest::gtest PRIVATE GTest::gtest_main)

set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

# Register test executable with ctest
add_test(
        NAME ${PROJECT_NAME}
        COMMAND ${PROJECT_NAME}
)

# Copy over dependencies to test directory after build
get_target_property(shift_RUNTIME_OUTPUT_DIR shift RUNTIME_OUTPUT_DIRECTORY)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${shift_RUNTIME_OUTPUT_DIR} .
)