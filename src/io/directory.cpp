#include "../shift_config.h"

#include "directory.h"
#include "drive.h"
#include "file.h"
#include "../stdutils.h"
#include "io_exception.h"

namespace shift {
	namespace io {
		const std::wstring& directory::m_check_path(const std::wstring& path) {
			if (this->exists())
				if (!std::filesystem::is_directory(std::filesystem::path(std::wstring(path)))) {
					throw io_exception("Path " + str_to_str<wchar_t, char>(path) + " is not a directory");
				}
			return path;
		}

		const std::filesystem::path& directory::m_check_path(const std::filesystem::path& path) {
			if (this->exists())
				if (!std::filesystem::is_directory(path)) {
					throw io_exception("Path " + str_to_str<wchar_t, char>(path.wstring()) + " is not a directory");
				}
			return path;
		}

		SHIFT_FILESYSTEM_API directory::directory(const char* path) : directory(std::string(path)) {
		}

		SHIFT_FILESYSTEM_API directory::directory(const wchar_t* path) : directory(std::wstring(path)) {
		}

		SHIFT_FILESYSTEM_API directory::directory(const std::string& path) : m_path(str_to_str<char, wchar_t>(path)) {
			this->m_init();
		}

		SHIFT_FILESYSTEM_API directory::directory(const std::wstring& wpath) : m_path(std::wstring(wpath)) {
			this->m_init();
		}

		SHIFT_FILESYSTEM_API directory::directory(std::wstring&& wpath) : m_path(std::move(wpath)) {
			this->m_init();
		}

		SHIFT_FILESYSTEM_API directory::directory(const std::filesystem::path& path) : m_path(path) {
			this->m_init();
		}

		SHIFT_FILESYSTEM_API directory::directory(std::filesystem::path&& path)
		noexcept(std::is_nothrow_move_constructible<std::filesystem::path>::value) : m_path(std::move(path)) {
			this->m_init();
		}

		SHIFT_FILESYSTEM_API directory::directory(const directory& other) : m_path(other.m_path) {
			this->m_init();
		}

		SHIFT_FILESYSTEM_API directory::directory(directory&& other) noexcept(std::is_nothrow_move_constructible<std::filesystem::path>::value) : m_path(
				std::move(other.m_path)) {
			this->m_init();
		}

		SHIFT_FILESYSTEM_API directory::~directory(void) {
		}

		SHIFT_FILESYSTEM_API directory& directory::operator=(directory&& other) noexcept {
			this->m_path = std::move(other.m_path);
			this->m_init();
			return *this;
		}

		SHIFT_FILESYSTEM_API directory& directory::operator=(const directory& other) {
			this->m_path = other.m_path;
			this->m_init();
			return *this;
		}

		SHIFT_FILESYSTEM_API bool directory::exists(void) const noexcept {
			try {
				return std::filesystem::exists(this->m_path);
			} catch (...) {
				return false;
			}
		}

		SHIFT_FILESYSTEM_API bool directory::mkdir(const bool parent) noexcept {
			try {
				if (this->exists())
					return true;
				if (parent)
					return std::filesystem::create_directories(this->m_path);
				else
					return std::filesystem::create_directory(this->m_path);
			} catch (...) {
				return false;
			}
		}

		SHIFT_FILESYSTEM_API bool directory::remove(void) noexcept {
			try {
				if (!this->exists())
					return false;
				return std::filesystem::remove_all(this->m_path);
			} catch (...) {
				return false;
			}
		}

		SHIFT_FILESYSTEM_API drive directory::get_drive(void) const {
			// Not working on UNIX, refer to: https://unix.stackexchange.com/questions/34858/what-is-the-concept-of-drives-in-unix-systems

			// Full working on Windows? We must assume this is a drive, no UNC network path stuff
			return drive::get_drive(std::filesystem::absolute(this->m_path).root_path().u8string().at(0));
		}

		SHIFT_FILESYSTEM_API directory directory::get_parent(void) const {
			return directory(std::filesystem::absolute(this->m_path).parent_path());
		}

		SHIFT_FILESYSTEM_API std::list<directory> directory::get_subdirectories(void) const noexcept {
			std::list<directory> ret;
			for (const auto& path : std::filesystem::directory_iterator(this->m_path)) {
				if (path.is_directory())
					ret.push_back(path.path());
			}
			return ret;
		}

		SHIFT_FILESYSTEM_API std::list<file> directory::get_subfiles(void) const noexcept {
			std::list<file> ret;
			for (const auto& path : std::filesystem::directory_iterator(this->m_path)) {
				if (path.is_regular_file() || path.is_symlink()) {
					ret.push_back(path.path());
				}
			}
			return ret;
		}

		SHIFT_FILESYSTEM_API directory directory::get_current_path(void) {
			return directory(std::filesystem::current_path());
		}

		SHIFT_FILESYSTEM_API std::wstring directory::name(void) const noexcept {
			return this->m_path.filename().wstring();
		}

		SHIFT_FILESYSTEM_API std::filesystem::path& directory::get_path(void) {
			this->m_init();
			return this->m_path;
		}

		SHIFT_FILESYSTEM_API const std::filesystem::path& directory::get_path(void) const noexcept {
			return this->m_path;
		}

		SHIFT_FILESYSTEM_API std::uintmax_t directory::size(void) const {
			std::uintmax_t size = 0;

			for (const directory& dir : this->get_subdirectories()) {
				size += dir.size();
			}
			for (const file& file : this->get_subfiles()) {
				size += file.size();
			}

			return size;
		}

		SHIFT_FILESYSTEM_API std::filesystem::path directory::absolute_path(void) const {
			return std::filesystem::absolute(this->m_path);
		}

		void directory::m_init(void) {
			this->m_path.make_preferred();
			const std::filesystem::path::iterator& it = (--this->m_path.end());
			if (it->native().size() <= 0) {
				this->m_path = this->m_path.parent_path();
			}

			this->m_check_path(this->m_path);
		}

		SHIFT_FILESYSTEM_API bool directory::operator==(const directory& other) const noexcept {
			return this->m_path == other.m_path;
		}

		SHIFT_FILESYSTEM_API bool directory::operator==(const std::string& str) const noexcept {
			return this->m_path.compare(str_to_str<char, wchar_t>(str)) == 0;
		}

		SHIFT_FILESYSTEM_API bool directory::operator==(const std::wstring& wstr) const noexcept {
			return this->m_path.compare(wstr) == 0;
		}

		SHIFT_FILESYSTEM_API io::file directory::operator/(const io::file& file) const {
			return io::file(this->m_path / file.get_path());
		}

		SHIFT_FILESYSTEM_API io::directory directory::operator/(const io::directory& dir) const {
			return io::directory(this->m_path / dir.m_path);
		}

		SHIFT_FILESYSTEM_API io::directory& directory::operator/=(const io::directory& dir) {
			this->m_path /= dir.m_path;
			return *this;
		}

		SHIFT_FILESYSTEM_API directory::operator bool(void) const noexcept {
			try {
				return this->exists();
			} catch (...) {
				return false;
			}
		}

		SHIFT_FILESYSTEM_API bool directory::operator==(const char* str) const noexcept {
			return this->m_path.compare(str_to_str<char, wchar_t>(str)) == 0;
		}

		SHIFT_FILESYSTEM_API bool directory::operator==(const wchar_t* wstr) const noexcept {
			return this->m_path.compare(wstr) == 0;
		}

	}
}
