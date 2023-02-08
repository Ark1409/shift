/**
 * @file filesystem/drive.h
 *
 * Represents a drive in the windows file system
 */
#ifndef SHIFT_FILESYSTEM_DRIVE_H_
#define SHIFT_FILESYSTEM_DRIVE_H_ 1

#include "shift_config.h"

#ifndef SHIFT_SUBSYSTEM_WINDOWS
#	error drive.h may only be included on Windows platforms
#endif

#include <list>
#include <windows.h>
#include <ostream>

namespace shift {
	namespace filesystem {
		class drive {
		public:
			/**
			 * Retrieves the logical drives associated with the underlying letter.
			 * If the underlying drive letter does not exist, this function returns an invalid drive.
			 *
			 * @param[in] letter Drive letter
			 * @return Corresponding drive, may be @c invalid.
			 */
			static inline drive get_drive(char letter) { return drive(letter); }

			/**
			 * Retrieves the system drive (the drive on which Windows is installed)
			 * @return The system drive.
			 */
			static inline drive get_system_drive(void) noexcept { return drive(); }

			/**
			 * Retrieves all logical drives on the system.
			 * @return A list of system drives.
			 */
			static std::list<drive> get_drives(void) noexcept;

			/**
			 * Retrieves the drive letter. This will ALWAYS be in upper case.
			 * @return The current drive letter.
			 */
			inline char get_letter(void) const noexcept { return this->m_letter; }

			/**
			 * Retrieves validity of the current drive object.
			 */
			inline operator bool(void) const noexcept { return !this->is_invalid(); }

			/**
			 * DWORD representation of the drive
			 * @see GetLogicalDrives()
			 */
			inline DWORD dword_val(void) const noexcept {
				if (this->is_invalid()) return 0x0;
				return (DWORD)(1 << ((this->m_letter) - (char)'A'));
			}

			/**
			 * Indicates whether the drive is invalid.
			 *
			 * @return Returns true if the underlying drive is invalid, false otherwise.
			 */
			inline bool is_invalid(void) const noexcept { return this->m_letter == 0x0; }
		protected:
			char m_letter = 0x0; // Representing invalid drive
			drive(char) noexcept; // Drive from letter
		private:
			void m_init(const bool sys = false) noexcept; // Drive initialization
			drive(void) noexcept; // System drive
		};

	}
}

template<typename _CharT, typename _Traits>
inline std::basic_ostream<_CharT, _Traits>& operator<<(std::basic_ostream<_CharT, _Traits>& __os, const shift::filesystem::drive __drive) {
	return __os << _CharT(__drive.get_letter()) << _CharT(':') << _CharT('\\');
}

#endif /* SHIFT_FILESYSTEM_DRIVE_H_ */
