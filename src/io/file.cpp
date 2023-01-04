#include "file.h"
#include "drive.h"
#include "directory.h"
#include "../stdutils.h"
#include "io_exception.h"
#include <fstream>

#define is_file(__path) (std::filesystem::is_regular_file((__path))||std::filesystem::is_symlink((__path)))

namespace shift {
	namespace io {
		SHIFT_FILESYSTEM_API file::file(const char* path) : file(std::string(path)) {
		}

		SHIFT_FILESYSTEM_API file::file(const wchar_t* path) : file(std::wstring(path)) {
		}

		SHIFT_FILESYSTEM_API file::file(const std::string& path) : m_path(str_to_str<char, wchar_t>(path)) {
			this->m_init();
		}

		SHIFT_FILESYSTEM_API file::file(const std::wstring& path) : m_path(path) {
			this->m_init();
		}

		SHIFT_FILESYSTEM_API file::file(std::wstring&& path) : m_path(std::move(path)) {
			this->m_init();
		}

		SHIFT_FILESYSTEM_API file::file(const std::filesystem::path& path) : m_path(path) {
			this->m_init();
		}

		SHIFT_FILESYSTEM_API file::file(std::filesystem::path&& path) : m_path(std::move(path)) {
			this->m_init();
		}

		SHIFT_FILESYSTEM_API file::file(const file& other) : m_path(other.m_path) {
			this->m_init();
		}

		SHIFT_FILESYSTEM_API file::file(file&& other) : m_path(std::move(other.m_path)) {
			this->m_init();
		}

		SHIFT_FILESYSTEM_API bool file::operator==(const file& other) const noexcept {
			return (this->m_path == other.m_path) || (std::filesystem::absolute(this->m_path) == std::filesystem::absolute(other.m_path));
		}

		SHIFT_FILESYSTEM_API bool file::operator==(const char* str) const noexcept {
			return this->m_path.compare(str_to_str<char, wchar_t>(str)) == 0;
		}

		SHIFT_FILESYSTEM_API bool file::operator==(const wchar_t* wstr) const noexcept {
			return this->m_path.compare(wstr) == 0;
		}

		SHIFT_FILESYSTEM_API bool file::operator==(const std::string& str) const noexcept {
			return this->m_path.compare(str_to_str<char, wchar_t>(str)) == 0;
		}

		SHIFT_FILESYSTEM_API bool file::operator==(const std::wstring& wstr) const noexcept {
			return this->m_path.compare(wstr) == 0;
		}

		SHIFT_FILESYSTEM_API file::operator bool(void) const noexcept {
			try {
				return this->exists();
			} catch (...) {
				return false;
			}
		}

		SHIFT_FILESYSTEM_API file::operator const std::filesystem::path&(void) const noexcept {
			return this->get_path();
		}

		SHIFT_FILESYSTEM_API file::operator std::filesystem::path&(void) noexcept {
			return this->get_path();
		}

		SHIFT_FILESYSTEM_API drive file::get_drive(void) const {
			// Not working on UNIX, refer to: https://unix.stackexchange.com/questions/34858/what-is-the-concept-of-drives-in-unix-systems

			// Full working on Windows? We must assume this is a drive, no UNC network path stuff
			return drive::get_drive(std::filesystem::absolute(this->m_path).root_path().u8string().at(0));
		}

		SHIFT_FILESYSTEM_API directory file::get_parent(void) const {
			return std::filesystem::absolute(this->m_path).parent_path();
		}

		SHIFT_FILESYSTEM_API bool file::create(bool make_parent) noexcept {
			try {
				// No need to create the file if it already exists
				if (this->exists())
					return true;

				// Create parent directories, if set to do so
				if (make_parent && !this->get_parent().exists())
					this->get_parent().mkdir(true);

				// Create file by using std::ofstream
				std::ofstream out(std::filesystem::absolute(this->m_path.make_preferred()), std::ios_base::out);
				out.close();

				// Return whether the current file has been created by checking if it exists
				return this->exists();
			} catch (...) {
				return false;
			}
		}

		SHIFT_FILESYSTEM_API bool file::remove(void) {
			try {
				if (!this->exists())
					return true;
				return std::filesystem::remove(this->m_path);
			} catch (...) {
				return false;
			}
		}

		SHIFT_FILESYSTEM_API bool file::exists(void) const noexcept {
			try {
				return std::filesystem::exists(this->m_path);
			} catch (...) {
				return false;
			}
		}

		SHIFT_FILESYSTEM_API std::wstring file::extension(void) const noexcept {
			return this->m_path.extension().wstring();
		}

		SHIFT_FILESYSTEM_API std::wstring file::name(void) const noexcept {
			return this->m_path.filename();
		}

		const std::wstring& file::m_check_path(const std::wstring& path) {
			if (this->exists())
				if (!is_file(std::filesystem::path(std::wstring(path)))) {
					throw io_exception("Path " + str_to_str<wchar_t, char>(path) + " is not a file");
				}
			return path;
		}

		const std::filesystem::path& file::m_check_path(const std::filesystem::path& path) {
			if (this->exists())
				if (!is_file(path)) {
					throw io_exception("Path " + str_to_str<wchar_t, char>(path.wstring()) + " is not a file");
				}
			return path;
		}

		void file::m_init(void) {
			this->m_path.make_preferred();
			const std::filesystem::path::iterator it = (--this->m_path.end());
			if (it->native().size() <= 0) {
				this->m_path = this->m_path.parent_path();
			}

			this->m_check_path(this->m_path);
		}

		SHIFT_FILESYSTEM_API std::filesystem::path& file::get_path(void) {
			this->m_init();
			return this->m_path;
		}

		SHIFT_FILESYSTEM_API const std::filesystem::path& file::get_path(void) const noexcept {
			return this->m_path;
		}

		SHIFT_FILESYSTEM_API std::filesystem::path file::absolute_path(void) const {
			return std::filesystem::absolute(this->m_path);
		}

		SHIFT_FILESYSTEM_API std::uintmax_t file::size(void) const {
			return std::filesystem::file_size(this->m_path);
		}
	}
}
