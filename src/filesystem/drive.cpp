#include "shift_config.h"

#ifdef SHIFT_SUBSYSTEM_WINDOWS
#include "drive.h"
#include "utils/utils.h"

#include <filesystem>

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

	SHIFT_API std::list<drive> drive::get_drives(void) noexcept {
		std::list<drive> ret;
		DWORD const drives = GetLogicalDrives();
		for (char c = 'A'; c <= 'Z'; c++) {
			if (drives & ((DWORD)(1 << ((c)-(char)'A'))))
				ret.push_back(c);
		}
		return ret;
	}

	void drive::m_init(const bool is_system_drive) noexcept {
		if (is_system_drive) {
			// C:\Windows\System32 = 19 characters + NULL
			constexpr UINT read_count = 50;  // 50 characters should be enough
			WCHAR __buf[read_count]; // 50 characters should be enough
			UINT const len = GetSystemDirectoryW(__buf, read_count);
			this->m_letter = len > 0 ? std::toupper(std::filesystem::path(std::wstring(__buf, len)).root_name().string().at(0)) : INVALID_DRIVE;
		} else {
			if (!drive_exists(this->m_letter))
				this->m_letter = INVALID_DRIVE;
		}
	}

}

#endif /* SHIFT_SUBSYSTEM_WINDOWS */
