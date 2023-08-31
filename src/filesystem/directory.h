/**
 * @file filesystem/directory.h
 *
 * Represents a directory in the file system
 */
#ifndef SHIFT_FILESYSTEM_DIRECTORY_H_
#define SHIFT_FILESYSTEM_DIRECTORY_H_ 1

#include "shift_config.h"
#include "filesystem/file.h" // May cause include loop

#ifdef SHIFT_SUBSYSTEM_WINDOWS
#	include "filesystem/drive.h"
#endif

#include <filesystem>
#include <list>
#include <string>
#include <string_view>
#include <type_traits>
#include <locale>
#include <codecvt>

 /// A namespace representing the file system
namespace shift::filesystem {

	/**
	 * Represents a directory in the file system
	 */
	class directory {
	public:
		/// Represents the character separator used to identify different hierarchical levels in a directory system.
		static const char separator;
		//static constexpr char separator = '\\';
	public:
		/**
		 * @brief Creates a directory, assigning it the specified path.
		 *
		 * Creates a directory, assigning it the specified path.
		 *
		 * @param[in] path The path for this directory, as a string_view.
		 * @throw io_exception If the underlying directory is not a path
		 */
		inline directory(const std::string_view path);

		/**
		 * @brief Creates a directory, assigning it the specified path.
		 *
		 * Creates a directory, assigning it the specified path.
		 *
		 * @param[in] path The path for this directory, as a string.
		 * @throw io_exception If the underlying directory is not a path
		 */
		inline directory(const std::string& path);

		/**
		 * @brief Creates a directory, assigning it the specified path.
		 *
		 * Creates a directory, assigning it the specified path.
		 *
		 * @param[in] path The path for this directory, as a std::filesystem::path.
		 * @throw io_exception If the underlying directory is not a path
		 */
		inline directory(const std::filesystem::path& path);

		/**
		 * @brief Creates a directory, assigning it the specified path.
		 *
		 * Creates a directory, assigning it the specified path.
		 *
		 * @param[in] path The path for this directory, as a std::filesystem::path.
		 * @throw io_exception If the underlying directory is not a path
		 */
		inline directory(std::filesystem::path&& path) noexcept;

		/**
		 * @brief Creates a directory with the same content as the specified directory.
		 *
		 * Creates a directory with the same content as the specified directory.
		 *
		 * @param[in] path The directory to copy.
		 */
		directory(const directory&) = default;

		/**
		 * @brief Creates a directory with the same content as the specified directory.
		 *
		 * Creates a directory with the same content as the specified directory.
		 *
		 * @param[in] path The directory to move.
		 */
		directory(directory&&) noexcept = default;

		/**
		 * Releases all resources linked to the current directory object
		 */
		~directory() noexcept = default;

		/**
		 * Re-assigns the current directory.
		 *
		 * @param[in] dir The directory to move.
		 * @return Current object, assigned with a new path.
		 */
		directory& operator=(const directory&) = default;

		/**
		 * Re-assigns the current directory.
		 *
		 * @param[in] path The directory to move.
		 * @return Current object, assigned with a new path.
		 */
		directory& operator=(directory&&) noexcept = default;

		/**
		 * Checks whether the two directories are equal.
		 * @param[in] dir The directory to compare, as a std::filesystem::path
		 * @return True if both directories are the same, false otherwise.
		 */
		SHIFT_API bool operator==(const std::filesystem::path& dir) const noexcept;

		/**
		 * Checks whether the two directories are equal.
		 * @param[in] dir The directory to compare.
		 * @return True if both directories are the same, false otherwise.
		 */
		inline bool operator==(const directory& dir) const noexcept { return operator==(dir.m_path); }

		/**
		 * Checks whether the two directories are equal.
		 * @param[in] path The directory to compare.
		 * @return True if both directories are the same, false otherwise.
		 */
		inline bool operator==(const std::string_view dir) const noexcept { return operator==(std::filesystem::path(dir)); }

		/**
		 * Checks whether the two directories are equal.
		 * @param[in] path The directory to compare.
		 * @return True if both directories are the same, false otherwise.
		 */
		inline bool operator==(const std::string& dir) const noexcept { return operator==(std::filesystem::path(dir)); }
		inline bool operator!=(const directory& dir) const noexcept { return !operator==(dir); }
		inline bool operator!=(const std::string_view dir) const noexcept { return !operator==(dir); }
		inline bool operator!=(const std::string& dir) const noexcept { return !operator==(dir); }

		/**
		 * Prepends the currently directory path to the underlying file.
		 * @param[in] file The file to append to the current directory.
		 * @return A file containing the new path.
		 */
		inline file operator/(const file& file) const { return filesystem::file(this->m_path / file.raw_path()); }


		/**
		 * Prepends the currently directory path to the underlying directory.
		 * @param[in] dir The directory to append to the current directory.
		 * @return A directory containing the new path.
		 */
		inline directory operator/(const directory& dir) const { return directory(this->m_path / dir.m_path); }

		/**
		 * Appends the underlying directory path to the current one.
		 * @param[in] dir The directory to append to the current directory.
		 * @return A directory containing the new path (current instance).
		 */
		inline directory& operator/=(const directory& dir) {
			this->m_path /= dir.m_path;
			return *this;
		}

		/**
		 * Returns @c true if the directory exists, @c false otherwise.
		 */
		inline operator bool(void) const noexcept { return this->exists(); }

		/**
		 * Checks whether the current directory exists.
		 * @return Returns true if the directory exists; false otherwise.
		 */
		SHIFT_API bool exists(void) const noexcept;

		/**
		 * Creates the current directory inside the file system.
		 *
		 * @param[in, optional] parents Whether to also create parent directory if they have not yet been created. True by default.
		 * @return True if the directory/directories were successfully created, false otherwise.
		 */
		SHIFT_API bool mkdir(const bool parents = true) noexcept;

		/**
		 * Deletes the directory.
		 *
		 * @return True if the directory was successfully deleted, false otherwise.
		 */
		SHIFT_API bool remove(void) noexcept;

		/**
		 * Gets the parent directory of this directory, returning the current directory if it has no parent.
		 * @return The parent directory
		 */
		inline directory get_parent(void) const { return directory(std::filesystem::absolute(std::filesystem::weakly_canonical(this->m_path)).parent_path()); }

		/**
		 * Retrieves all direct children directories to the current one.
		 *
		 * @return The current directory's sub-directories
		 */
		SHIFT_API std::list<directory> get_subdirectories(void) const;

		/**
		 * Retrieves all direct children files to the current directory.
		 *
		 * @return The current directory's sub-files.
		 */
		SHIFT_API std::list<file> get_subfiles(void) const;

		/**
		 * Retrieves an lvalue reference to the std::filesystem::path linked to the current directory.
		 *
		 * @return The path linked to this directory, as a std::filesystem::path
		 */
		inline std::filesystem::path& raw_path(void) noexcept { return this->m_path; }

		/**
		 * Retrieves a const lvalue reference to the std::filesystem::path linked to the current directory.
		 *
		 * @return The path linked to this directory, as a std::filesystem::path
		 */
		inline const std::filesystem::path& raw_path(void) const noexcept { return this->m_path; }

		/**
		 * Retrieves the name of the current directory.
		 * @return The current directory name.
		 */
		inline std::string get_name(void) const { return this->m_path.filename().string(); }

		/**
		 * Retrieves the size of all the directory's content's combined (sub-files and sub-directories).
		 * @return The size of the directory.
		 */
		SHIFT_API std::uintmax_t get_size(void) const;
		inline std::uintmax_t size(void) const { return get_size(); }

		/**
		 * Retrieves a std::filesystem::path containing the absolute path of the current directory.
		 * @return The absolute path of the current directory.
		 */
		inline std::string get_absolute_path(void) const { return std::filesystem::absolute(this->m_path).string(); }

		/**
		 * Retrieves a std::filesystem::path containing the absolute path of the current directory.
		 * @return The absolute path of the current directory.
		 */
		inline std::string get_canonical_path(void) const { return std::filesystem::weakly_canonical(this->m_path).string(); }
		inline directory get_absolute_directory(void) const { return directory(this->get_absolute_path()); }
		inline directory get_canonical_directory(void) const { return directory(this->get_canonical_path()); }

		/**
		 * Retrieves the current working directory.
		 *
		 * @return The process' current path, also known as working directory.
		 */
		inline static directory get_current_path(void) { return directory(std::filesystem::current_path()); }
		inline static directory get_current_directory(void) { return get_current_path(); }

#ifdef SHIFT_SUBSYSTEM_WINDOWS
		inline drive get_drive(void) const {
			// Not working on UNIX, refer to: https://unix.stackexchange.com/questions/34858/what-is-the-concept-of-drives-in-unix-systems

			// Full working on Windows? We must assume this is a drive, no UNC network path stuff
			return drive::get_drive(std::filesystem::absolute(this->m_path).root_path().string().at(0));
		}
#endif
	protected:
		std::filesystem::path m_path; // Current directory path
	};

	inline directory::directory(const std::string_view path) : m_path(path) {}
	inline directory::directory(const std::string& path) : m_path(path) {}
	inline directory::directory(const std::filesystem::path& path) : m_path(path) {}
	inline directory::directory(std::filesystem::path&& path) noexcept : m_path(std::move(path)) {}

	inline const char directory::separator = std::wstring_convert<std::codecvt_utf8<std::filesystem::path::value_type>, std::filesystem::path::value_type>().to_bytes(std::filesystem::path::preferred_separator).at(0);

}

/// Overload allowing directories to directly be written into output streams
template<typename _CharT, typename _Traits>
inline std::basic_ostream<_CharT, _Traits>& operator<<(std::basic_ostream<_CharT, _Traits>& __os, const shift::filesystem::directory& __dir) {
	return __os << __dir.raw_path().string<_CharT>();
}

#endif /* SHIFT_FILESYSTEM_DIRECTORY_H_ */
