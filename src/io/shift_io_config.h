/**
 * @file include/shift_file_config.h
 *
 * This header file must not be included directly, and should be included through its parent header file, @a shift_config.h,
 * for proper functionality.
 */

#ifndef SHIFT_FILE_CONFIG_H_
#define SHIFT_FILE_CONFIG_H_ 1

// Directory separators (e.g. "C:\Windows\System32")
#ifdef SHIFT_SUBSYSTEM_WINDOWS
#	define SHIFT_DIRECTORY_SEPARATOR '\\' // Windows directory separator
#elif defined (SHIFT_SUBSYSTEM_MAC_OSX)
#	define SHIFT_DIRECTORY_SEPARATOR '/' // MacOS directory separator
#elif defined(SHIFT_SUBSYSTEM_LINUX)
#	define SHIFT_DIRECTORY_SEPARATOR '/' // Linux directory separator
#elif defined(SHIFT_SUBSYSTEM_UNIX)
#	define SHIFT_DIRECTORY_SEPARATOR '/' // Unix directory separator
#else
#	define SHIFT_DIRECTORY_SEPARATOR '/' // Operating system not known
#endif


// Path separators (e.g. "%JAVA_HOME%\bin;C:\Program Files (x86)\dotnet\;C:\Program Files\Git\cmd")
#ifdef SHIFT_SUBSYSTEM_WINDOWS
#	define SHIFT_PATH_SEPARATOR ';' // Windows path separator
#elif defined(SHIFT_SUBSYSTEM_MAC_OSX)
#	define SHIFT_PATH_SEPARATOR ':' // MacOS path separator
#elif defined(SHIFT_SUBSYSTEM_LINUX)
#	define SHIFT_PATH_SEPARATOR ':' // Linux path separator
#elif defined(SHIFT_SUBSYSTEM_UNIX)
#	define SHIFT_PATH_SEPARATOR ':' // Unix path separator
#else
#	define SHIFT_PATH_SEPARATOR ':' // Operating system not known
#endif

#define SHIFT_FILESYSTEM_API SHIFT_API

#endif /* SHIFT_FILE_CONFIG_H_ */
