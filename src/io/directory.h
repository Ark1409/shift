#ifndef SHIFT_FILESYSTEM_DIRECTORY_H_
#define SHIFT_FILESYSTEM_DIRECTORY_H_ 1

#include "../shift_config.h"

#include <filesystem>
#include <list>
#include <string>
#include <type_traits>

/// A namespace representing the file system
namespace shift {
	namespace io {
		class drive;
		// Drive class from drive.h
		class file;
		// File class from file.h

		/**
		 Represents a directory in the file system
		 */
		class SHIFT_FILESYSTEM_API directory {
			public:
				/// Represents the character separator used to identify different hierarchical levels in a directory system.
				SHIFT_FILESYSTEM_API static constexpr char separator = SHIFT_DIRECTORY_SEPARATOR;
			protected:
				std::filesystem::path m_path; // Current directory path
				/// Public members
			public:
				/**
				 * @brief Creates a directory, assigning it the specified path.
				 *
				 * Creates a directory, assigning it the specified path.
				 *
				 * @param[in] __path The path for this directory, as a char array.
				 * @throw io_exception If the underlying directory is not a path
				 */
				directory(const char*);

				/**
				 * @brief Creates a directory, assigning it the specified path.
				 *
				 * Creates a directory, assigning it the specified path.
				 *
				 * @param[in] __path The path for this directory, as a wchar_t array.
				 * @throw io_exception If the underlying directory is not a path
				 */
				directory(const wchar_t*);

				/**
				 * @brief Creates a directory, assigning it the specified path.
				 *
				 * Creates a directory, assigning it the specified path.
				 *
				 * @param[in] __path The path for this directory, as a string.
				 * @throw io_exception If the underlying directory is not a path
				 */
				directory(const std::string&);

				/**
				 * @brief Creates a directory, assigning it the specified path.
				 *
				 * Creates a directory, assigning it the specified path.
				 *
				 * @param[in] __path The path for this directory, as a wstring.
				 * @throw io_exception If the underlying directory is not a path
				 */
				directory(const std::wstring&);

				/**
				 * @brief Creates a directory, assigning it the specified path.
				 *
				 * Creates a directory, assigning it the specified path.
				 *
				 * @param[in] __path The path for this directory, as a wstring.
				 * @throw io_exception If the underlying directory is not a path
				 */
				directory(std::wstring&&);

				/**
				 * @brief Creates a directory, assigning it the specified path.
				 *
				 * Creates a directory, assigning it the specified path.
				 *
				 * @param[in] __path The path for this directory, as a std::filesystem::path.
				 * @throw io_exception If the underlying directory is not a path
				 */
				directory(const std::filesystem::path&);

				/**
				 * @brief Creates a directory, assigning it the specified path.
				 *
				 * Creates a directory, assigning it the specified path.
				 *
				 * @param[in] __path The path for this directory, as a std::filesystem::path.
				 * @throw io_exception If the underlying directory is not a path
				 */
				directory(std::filesystem::path&&) noexcept(std::is_nothrow_move_constructible<std::filesystem::path>::value);

				/**
				 * @brief Creates a directory with the same content as the specified directory.
				 *
				 * Creates a directory with the same content as the specified directory.
				 *
				 * @param[in] __path The directory to copy.
				 */
				directory(const directory&);

				/**
				 * @brief Creates a directory with the same content as the specified directory.
				 *
				 * Creates a directory with the same content as the specified directory.
				 *
				 * @param[in] __path The directory to move.
				 */
				directory(directory&&) noexcept(std::is_nothrow_move_constructible<std::filesystem::path>::value);

				/**
				 * Re-assigns the current directory.
				 *
				 * @param[in] __path The directory to move.
				 * @return Current object, assigned with a new path.
				 */
				directory& operator=(directory&&) noexcept;

				/**
				 * Re-assigns the current directory.
				 *
				 * @param[in] __path The directory to move.
				 * @return Current object, assigned with a new path.
				 */
				directory& operator=(const directory&);

				/**
				 * Checks whether the two directories are equal.
				 * @param[in] __path The directory to compare.
				 * @return True if both directories are the same, false otherwise.
				 */
				virtual bool operator==(const directory&) const noexcept;

				/**
				 * Checks whether the two directories are equal.
				 * @param[in] __path The directory to compare.
				 * @return True if both directories are the same, false otherwise.
				 */
				virtual bool operator==(const std::string&) const noexcept;

				/**
				 * Checks whether the two directories are equal.
				 * @param[in] __path The directory to compare.
				 * @return True if both directories are the same, false otherwise.
				 */
				virtual bool operator==(const std::wstring&) const noexcept;

				/**
				 * Checks whether the two directories are equal.
				 * @param[in] __path The directory to compare.
				 * @return True if both directories are the same, false otherwise.
				 */
				virtual bool operator==(const char*) const noexcept;

				/**
				 * Checks whether the two directories are equal.
				 * @param[in] __path The directory to compare.
				 * @return True if both directories are the same, false otherwise.
				 */
				virtual bool operator==(const wchar_t*) const noexcept;

				/**
				 * Prepends the currently directory path to the underlying file.
				 * @param[in] __file The file to append to the current directory.
				 * @return A file containing the new path.
				 */
				io::file operator/(const io::file&) const;

				/**
				 * Prepends the currently directory path to the underlying directory.
				 * @param[in] __dir The directory to append to the current directory.
				 * @return A directory containing the new path.
				 */
				io::directory operator/(const io::directory&) const;

				/**
				 * Appends the underlying directory path to the current one.
				 * @param[in] __dir The directory to append to the current directory.
				 * @return A directory containing the new path (current instance).
				 */
				io::directory& operator/=(const io::directory&);

				/**
				 * Returns @c true if the directory exists, @c false otherwise.
				 */
				operator bool(void) const noexcept;

				/**
				 * Releases all resources linked to the current directory object
				 */
				virtual ~directory(void);

				/**
				 * Retrieves the drive linked to the current directory.
				 * @return The drives linked to this directory.
				 */
				virtual drive get_drive(void) const;

				/**
				 * Checks whether the current directory exists.
				 * @return Returns true if the directory exists; false otherwise.
				 */
				virtual bool exists(void) const noexcept;

				/**
				 * Creates the current directory inside the file system.
				 *
				 * @param[in] parents Whether to also create parent directory if they have not yet been created. True by default.
				 * @return True if the directory/directories were successfully created, false otherwise.
				 */
				virtual bool mkdir(const bool parents = true) noexcept;

				/**
				 * Deletes the directory.
				 *
				 * @return True if the directory was successfully deleted, false otherwise.
				 */
				virtual bool remove(void) noexcept;

				/**
				 * Gets the parent directory of this directory, returning the current directory if it has no parent.
				 * @return The parent directory
				 */
				virtual directory get_parent(void) const;

				/**
				 * Retrieves all direct children directories to the current one.
				 *
				 * @return The current directory's sub-directories
				 */
				virtual std::list<directory> get_subdirectories(void) const noexcept;

				/**
				 * Retrieves all direct children files to the current directory.
				 *
				 * @return The current directory's sub-files.
				 */
				virtual std::list<file> get_subfiles(void) const noexcept;

				/**
				 * Retrieves the name of the current directory.
				 * @return The current directory name.
				 */
				virtual std::wstring name(void) const noexcept;

				/**
				 * Retrieves an lvalue reference to the std::filesystem::path linked to the current directory.
				 *
				 * @return The path linked to this directory, as a std::filesystem::path
				 */
				virtual std::filesystem::path& get_path(void);

				/**
				 * Retrieves a const lvalue reference to the std::filesystem::path linked to the current directory.
				 *
				 * @return The path linked to this directory, as a std::filesystem::path
				 */
				virtual const std::filesystem::path& get_path(void) const noexcept;

				/**
				 * Retrieves the size of all the directory's content's combined (sub-files and sub-directories).
				 * @return The size of the directory.
				 */
				virtual std::uintmax_t size(void) const;

				/**
				 * Retrieves a std::filesystem::path containing the absolute path of the current directory.
				 * @return The absolute path of the current directory.
				 */
				virtual std::filesystem::path absolute_path(void) const;

				inline directory absolute_dir(void) const {
					return directory(this->absolute_path());
				}

				/**
				 * Retrieves the current working directory.
				 *
				 * @return The process' current path, also known as working directory.
				 */
				static directory get_current_path(void);
			private:
				const std::wstring& m_check_path(const std::wstring&); // Checks if path for directory is valid
				const std::filesystem::path& m_check_path(const std::filesystem::path&); // Checks if path for directory is valid
				void m_init(void); // Initialize
		};

	}
}

template<typename _CharT, typename _Traits>
std::basic_ostream<_CharT, _Traits>& operator<<(std::basic_ostream<_CharT, _Traits>& __os, const shift::io::directory& __dir) {
	return __os << __dir.get_path().string<_CharT>();
}
#endif /* SHIFT_FILESYSTEM_DIRECTORY_H_ */
