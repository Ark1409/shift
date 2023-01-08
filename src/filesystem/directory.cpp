#include "shift_config.h"

#include "directory.h"
#include "drive.h"
#include "file.h"

namespace shift {
	namespace filesystem {
		 

		bool directory::exists(void) const noexcept {
			try {
				return std::filesystem::exists(this->m_path);
			}
			catch (...) {
				return false;
			}
		}

		bool directory::mkdir(const bool parent) noexcept {
			try {
				if (this->exists())
					return true;
				return parent ? std::filesystem::create_directories(this->m_path) : std::filesystem::create_directory(this->m_path);
			}
			catch (...) {
				return false;
			}
		}

		bool directory::remove(void) noexcept {
			try {
				if (!this->exists())
					return true;
				return std::filesystem::remove_all(this->m_path);
			}
			catch (...) {
				return false;
			}
		}

		std::list<directory> directory::get_subdirectories(void) const {
			std::list<directory> dirs;
			for (const auto& path : std::filesystem::directory_iterator(this->m_path)) {
				if (path.is_directory())
					dirs.push_back(path.path());
			}
			return dirs;
		}

		std::list<file> directory::get_subfiles(void) const {
			std::list<file> files;
			for (const auto& path : std::filesystem::directory_iterator(this->m_path)) {
				if (path.is_regular_file() || path.is_symlink())
					files.push_back(path.path());
			}
			return files;
		}

		std::uintmax_t directory::get_size(void) const {
			std::uintmax_t size = 0;

			for (const directory& dir : this->get_subdirectories())
				size += dir.size();
			for (const file& file : this->get_subfiles())
				size += file.size();

			return size;
		}

		bool directory::operator==(const std::filesystem::path& path) const noexcept {
			try {
				return std::filesystem::exists(path) ? std::filesystem::is_directory(path) && std::filesystem::weakly_canonical(this->m_path) == std::filesystem::weakly_canonical(path)
					: std::filesystem::weakly_canonical(this->m_path) == std::filesystem::weakly_canonical(path);
			}
			catch (...) {
				return false;
			}
		}
	}
}
