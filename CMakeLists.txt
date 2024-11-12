# CMake 3.21 required to build
cmake_minimum_required(VERSION 3.21)

# Needed for project VERSION
cmake_policy(SET CMP0048 NEW)

# Set the project with latest version
project(shift VERSION 0.1.1)

# Add option as to whether tests should be built (requires googletest download)
option(SHIFT_TEST "Build shift tests" ON)

# C++ settings

# Create the compiler library for the project
add_library(
        ${PROJECT_NAME}
        SHARED
        src/compiler/shift_argument_parser.cpp
        src/compiler/shift_compiler.cpp
        src/compiler/error_handler.cpp
        src/compiler/parsing/parser.cpp
        src/compiler/lexing/lexer.cpp
        src/compiler/analyzing/analyzer.cpp
        src/filesystem/directory.cpp
        src/filesystem/drive.cpp
        src/filesystem/file.cpp
        src/logging/console.cpp
        src/utils/utils.cpp
        src/compiler/marker.h
        src/compiler/parsing/parser_fwd.h
        src/compiler/parsing/parser_types.h
        src/compiler/parsing/parser_types.cpp
        src/utils/iterator_wrapper.h
        src/compiler/analyzing/type_info.h
        src/compiler/analyzing/name_info.h
        src/compiler/analyzing/analyzer_fwd.h
        src/compiler/analyzing/type_info.cpp
        src/compiler/parsing/types.h
        src/compiler/parsing/types.cpp
        src/compiler/parsing/types_fwd.h
        src/compiler/parsing/parser_types_fwd.h
        src/utils/variant.h
        src/utils/type_traits.h
        src/utils/compare.h
        src/utils/containers.h
        src/utils/utility.h
        src/utils/strings.h
)

# Create base library for project
add_library("${PROJECT_NAME}::${PROJECT_NAME}" ALIAS ${PROJECT_NAME})

# shift can only compile with c++20 and later
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

# Add includes for the project
target_include_directories(${PROJECT_NAME} PUBLIC include src)

# Add warnings and compile options for the project, flags depend on compiler
# Clang has clang-cl which accepts MSVC flags, so we need to check for that
if((CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "GNU")
        OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Wwrite-strings -Wformat=2 -Wformat-security
            -Wformat-nonliteral -Wformat-y2k -Wcast-align -Wcast-qual -Walloca -Walloc-zero -Wstringop-overflow=4 -Wpacked
            -Wpointer-arith -Wredundant-decls -Wstack-protector -Wswitch-default -Wswitch -Wunreachable-code -Wunused
            -Wunused-parameter -Wmissing-format-attribute -Wmissing-noreturn -Wdisabled-optimization -fmessage-length=0)
elseif((CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "MSVC")
        OR (CMAKE_CXX_COMPILER_ID MATCHES "MSVC"))
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()

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

# Add target properties to shift compiler runner
target_link_libraries(${PROJECT_NAME}-runner PRIVATE ${PROJECT_NAME}::${PROJECT_NAME})
set_target_properties(${PROJECT_NAME}-runner PROPERTIES INTERPROCEDURAL_OPTIMIZATION True)
set_target_properties(${PROJECT_NAME}-runner PROPERTIES POSITION_INDEPENDENT_CODE True)
set_target_properties(${PROJECT_NAME}-runner PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

# Set output directory for the project
set_property(TARGET ${PROJECT_NAME}-runner ${PROJECT_NAME} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set_property(TARGET ${PROJECT_NAME}-runner ${PROJECT_NAME} PROPERTY LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set_property(TARGET ${PROJECT_NAME}-runner ${PROJECT_NAME} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

# Configure test project if testing is enabled
if(SHIFT_TEST)
    # Enable ctest
    enable_testing()

    # Add test subdirectory
    add_subdirectory(test)
endif()
