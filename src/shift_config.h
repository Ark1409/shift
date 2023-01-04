/**
 * @file include/shift_config.h
 *
 * Configuration file for the Shift compiler
 */

#ifndef SHIFT_CONFIG_H_
#define SHIFT_CONFIG_H_ 1

#if defined(_WIN32) || defined(WIN32) || defined (__WIN32) || defined(_WIN64)
#	define SHIFT_SUBSYSTEM_WINDOWS 1 // Compiling for Windows
#elif defined (__linux__) || defined(linux)
#	define SHIFT_SUBSYSTEM_LINUX 1 // Compiling for Linux
#elif defined(unix) || defined(__unix) || defined(__unix__)
#	define SHIFT_SUBSYSTEM_UNIX 1 // Compiling for a *nix operating system
#elif defined(__APPLE__) || defined(__MACH__)
#	define SHIFT_SUBSYSTEM_MAC_OSX 1 // Compiling for MacOS
#else
#	define SHIFT_SUBSYSTEM_UNDEFINED 1 // Compiling for unknown operating system
/* #	error Cannot compile Shift for an unknown operating system. */
#endif

#ifdef SHIFT_SUBSYSTEM_WINDOWS
	// Windows
#	define SHIFT_EXPORT __declspec(dllexport)
#	define SHIFT_IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
    //  GCC
    #define SHIFT_EXPORT __attribute__((visibility("default")))
    #define SHIFT_IMPORT
#else
	// Other
#	define SHIFT_EXPORT
#	define SHIFT_IMPORT
#endif

#if defined(SHIFT_BUILD_DLL) && defined(SHIFT_BUILD_STATIC)
#	error Cannot build Shift for both DLL and STATIC library
#elif defined(SHIFT_BUILD_DLL)
#	ifdef SHIFT_BUILD
#		define SHIFT_API SHIFT_EXPORT
#	else
#		define SHIFT_API SHIFT_IMPORT
#	endif
#elif defined(SHIFT_BUILD_STATIC)
#	define SHIFT_API
#else
#	error Either SHIFT_BUILD_STATIC or SHIFT_BUILD_DLL must be defined
#endif

#include "io/shift_io_config.h"
#include "compiler/shift_compiler_config.h"

#endif /* SHIFT_CONFIG_H_ */
