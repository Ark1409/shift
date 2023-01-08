/**
 * @file filesystem/file.h
 *
 * Represents a file in the file system
 */
#ifndef SHIFT_FILESYSTEM_FILE_H_
#define SHIFT_FILESYSTEM_FILE_H_ 1

#include "shift_config.h"

#ifdef SHIFT_SUBSYSTEM_WINDOWS
#	include "drive.h"
#endif

#include <filesystem>
#include <ostream>
#include <string>
#include <string_view>

 /// A namespace representing the file system
namespace shift {
	namespace filesystem {
		/// Must be forward declared to avoid include loop
		class directory;

		/// Represents a file in the file system. Each file requires a path where it is or will be located.
		class file {
		public:
			file(const std::string_view path);
			file(const std::string& path);
			file(const std::filesystem::path& path);
			file(std::filesystem::path&& path) noexcept;
			file(const file&) = default;
			file(file&&) noexcept = default;
			~file() noexcept = default;

			file& operator=(const file&) = default;
			file& operator=(file&&) noexcept = default;

			bool operator==(const std::filesystem::path& path) const noexcept;
			inline bool operator==(const file& other) const noexcept { return operator==(other.m_path); }
			inline bool operator==(const std::string_view path) const noexcept { return operator==(std::filesystem::path(path)); }
			inline bool operator==(const std::string& path) const noexcept { return operator==(std::filesystem::path(path)); }
			inline bool operator!=(const file& file) const noexcept { return !operator==(file); }
			inline bool operator!=(const std::string_view file) const noexcept { return !operator==(file); }
			inline bool operator!=(const std::string& file) const noexcept { return !operator==(file); }

			inline operator bool(void) const noexcept { return this->exists(); }
			inline operator std::string(void) const { return this->get_path(); }
			inline operator const std::filesystem::path& (void) const noexcept { return this->raw_path(); }
			inline operator std::filesystem::path& (void) noexcept { return this->raw_path(); }

			directory get_directory(void) const;
			directory get_parent(void) const;

			bool create(const bool parent = true) noexcept;
			bool remove(void) noexcept;
			bool exists(void) const noexcept;

			inline std::filesystem::path& raw_path(void) noexcept { return this->m_path; }
			inline const std::filesystem::path& raw_path(void) const noexcept { return this->m_path; }

			inline std::string get_name(void) const { return this->m_path.filename().string(); }
			inline std::string get_extension(void) const { return this->m_path.extension().string(); }
			inline std::string get_path(void) const { return this->m_path.string(); }
			inline std::string get_absolute_path(void) const { return std::filesystem::absolute(this->m_path).string(); }
			inline std::string get_canonical_path(void) const { return std::filesystem::weakly_canonical(this->m_path).string(); }

			inline file get_absolute_file(void) const { return file(std::filesystem::absolute(this->m_path)); }
			inline file get_canonical_file(void) const { return file(std::filesystem::weakly_canonical(this->m_path)); }

			/**
			 * Retrieves the size of the file on disk.
			 */
			inline std::uintmax_t get_size(void) const { return std::filesystem::file_size(this->m_path); }
			inline std::uintmax_t size(void) const { return get_size(); }

#ifdef SHIFT_SUBSYSTEM_WINDOWS
			inline drive get_drive(void) const {
				// Not working on UNIX, refer to: https://unix.stackexchange.com/questions/34858/what-is-the-concept-of-drives-in-unix-systems

				// Full working on Windows? We must assume this is a drive, no UNC network path stuff
				return drive::get_drive(std::filesystem::absolute(this->m_path).root_path().string().at(0));
			}
#endif
		protected:
			/// The current file path
			std::filesystem::path m_path;
		};

		inline file::file(const std::string_view path): m_path(path) {}
		inline file::file(const std::string& path) : m_path(path) {}
		inline file::file(const std::filesystem::path& path) : m_path(path) {}
		inline file::file(std::filesystem::path&& path) noexcept: m_path(std::move(path)) {}
	}
}

template<typename _CharT, typename _Traits>
inline std::basic_ostream<_CharT, _Traits>& operator<<(std::basic_ostream<_CharT, _Traits>& __os, const shift::filesystem::file& __file) {
	return __os << __file.raw_path().string<_CharT>();
}

#endif /* SHIFT_FILESYSTEM_FILE_H_ */
