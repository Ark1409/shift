#ifndef SHIFT_FILESYSTEM_FILE_H_
#define SHIFT_FILESYSTEM_FILE_H_ 1

#include "../shift_config.h"

#include <filesystem>
#include <string>
#include <string_view>

/// A namespace representing the file system
namespace shift {
	namespace io {
		class drive;
		// Drive class from drive.h
		class directory;
		// Directory class from directory.h

		/// Represents a file in the file system. Each file requires a path where it is or will be located.
		class SHIFT_FILESYSTEM_API file {
				// Protected fields
			protected:
				// The current file path
				std::filesystem::path m_path;
			public:
				file(const char*);
				file(const wchar_t*);
				file(const std::string&);
				file(const std::wstring&);
				file(std::wstring&&);
				file(const std::filesystem::path&);
				file(std::filesystem::path&&);
				file(const file&);
				file(file&&);
				virtual ~file(void) = default;

				file& operator=(const file&) = default;
				file& operator=(file&&) = default;

				virtual bool operator==(const file&) const noexcept;
				virtual bool operator==(const char*) const noexcept;
				virtual bool operator==(const wchar_t*) const noexcept;
				virtual bool operator==(const std::string&) const noexcept;
				virtual bool operator==(const std::wstring&) const noexcept;

				virtual inline bool operator!=(const file& file) const noexcept {
					return !operator==(file);
				}

				virtual inline bool operator!=(const char* file) const noexcept {
					return !operator==(file);
				}

				virtual inline bool operator!=(const wchar_t* file) const noexcept {
					return !operator==(file);
				}

				virtual inline bool operator!=(const std::string& file) const noexcept {
					return !operator==(file);
				}

				virtual inline bool operator!=(const std::wstring& file) const noexcept {
					return !operator==(file);
				}

				operator bool(void) const noexcept;
				operator std::filesystem::path&(void) noexcept;
				operator const std::filesystem::path&(void) const noexcept;

				virtual drive get_drive(void) const;
				virtual directory get_parent(void) const;
				virtual bool create(bool parent = true) noexcept;
				virtual bool remove(void); // deletes the file
				virtual bool exists(void) const noexcept;
				virtual std::wstring extension(void) const noexcept;
				virtual std::wstring name(void) const noexcept;
				virtual std::filesystem::path& get_path(void);
				virtual const std::filesystem::path& get_path(void) const noexcept;
				virtual std::filesystem::path absolute_path(void) const;
				virtual std::uintmax_t size(void) const;

				inline file absolute_file(void) const {
					return file(this->absolute_path());
				}
			private:
				const std::wstring& m_check_path(const std::wstring&); // Checks if path for directory is valid
				const std::filesystem::path& m_check_path(const std::filesystem::path&); // Checks if path for directory is valid
				void m_init(void); // Initialize
		};
	}
}

template<typename _CharT, typename _Traits>
inline std::basic_ostream<_CharT, _Traits>& operator<<(std::basic_ostream<_CharT, _Traits>& __os, const shift::io::file& __file) {
	return __os << __file.get_path().string<_CharT>();
}

#endif /* SHIFT_FILESYSTEM_FILE_H_ */
