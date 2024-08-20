#include "shift_config.h"

#ifdef SHIFT_SUBSYSTEM_WINDOWS
#include "drive.h"
#include "utils/utils.h"

#include <filesystem>
#include <bit>

#define INVALID_DRIVE 0x0

static bool drive_exists(const char letter) noexcept {
	if (!(is_between_in(letter, 'A', 'Z') || is_between_in(letter, 'a', 'z')))
		return false;

	const DWORD drives = GetLogicalDrives();
	return drives & ((DWORD)(1 << ((toupper(letter)) - (char)'A')));
}

namespace shift::filesystem {
	SHIFT_API drive::drive(char __letter) noexcept : m_letter(std::toupper(__letter)) { m_init(false); }
	drive::drive(void) noexcept : m_letter(INVALID_DRIVE) { m_init(true); }

	SHIFT_API std::vector<drive> drive::get_drives(void) noexcept {
		std::vector<drive> ret;
		DWORD const drives = GetLogicalDrives();
		ret.reserve(std::popcount(drives));
		for (char c = 'A'; c <= 'Z'; c++) {
			if (drives & ((DWORD)(1 << ((c)-(char)'A'))))
				ret.push_back(c);
		}
		return ret;
	}

	void drive::m_init(const bool is_system_drive) noexcept {
		if (is_system_drive) {
			// C:\Windows\System32 = 19 characters + NULL
			constexpr UINT read_count = 64;  // 64 characters should be enough
			WCHAR __buf[read_count]; // 64 characters should be enough
			UINT const len = GetSystemDirectoryW(__buf, read_count);
			auto root_name = std::filesystem::path(std::wstring(__buf, len)).root_name().string();
			this->m_letter = root_name.size() != 2 || root_name[1] != ':' ? INVALID_DRIVE : std::toupper(root_name[0]);
		} else {
			if (!drive_exists(this->m_letter))
				this->m_letter = INVALID_DRIVE;
		}
	}

}

#endif /* SHIFT_SUBSYSTEM_WINDOWS */
