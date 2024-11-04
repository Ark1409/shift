#include "file.h"
#include "directory.h"

#include <fstream>
#include <stdexcept>

namespace shift::filesystem {

    SHIFT_API directory file::get_directory() const { return get_parent(); }

    SHIFT_API directory file::get_parent() const { return std::filesystem::weakly_canonical(this->m_path).parent_path(); }

    SHIFT_API bool file::create(const bool make_parent) noexcept {
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
        }
        catch (...) {
            return false;
        }
    }

    SHIFT_API bool file::remove() noexcept {
        try {
            // Return true if the file already doesn't exist
            if (!this->exists())
                return true;

            // Try and delete the file
            return std::filesystem::remove(this->m_path);
        }
        catch (...) {
            return false;
        }
    }

    SHIFT_API bool file::exists() const noexcept {
        try {
            return std::filesystem::exists(this->m_path);
        }
        catch (...) {
            return false;
        }
    }

    SHIFT_API bool file::operator==(const std::filesystem::path& path) const noexcept {
        try {
            return std::filesystem::exists(path) ? std::filesystem::is_regular_file(path) &&
                                                   std::filesystem::weakly_canonical(this->m_path) ==
                                                   std::filesystem::weakly_canonical(path)
                                                 : std::filesystem::weakly_canonical(this->m_path) ==
                                                   std::filesystem::weakly_canonical(path);
        }
        catch (...) {
            return false;
        }
    }

    SHIFT_API std::string file::read_fully(bool binary) const {

        std::ifstream in(this->m_path, std::ios_base::in | (binary ? std::ios_base::binary : std::ios_base::in));
        std::string ret;

        if (!in.is_open())
            throw std::runtime_error("Unable to open file for reading: " + this->get_absolute_path());

        in.seekg(0, std::ios_base::end);
        {
            auto pos = in.tellg();
            if (pos != -1) { ret.resize(pos); }
        }
        in.seekg(0, std::ios_base::beg);
        in.read(ret.data(), ret.size());
        ret.resize(in.gcount());
        char buf[1024];
        while (in.read(buf, sizeof(buf)) && !in.eof()) {
            ret.append(buf, in.gcount());
        }
        in.close();

        return ret;
    }
}
