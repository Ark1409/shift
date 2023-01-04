#ifndef SHIFT_FILESYSTEM_DRIVE_H_
#define SHIFT_FILESYSTEM_DRIVE_H_ 1

#include "../shift_config.h"

#ifndef SHIFT_SUBSYSTEM_WINDOWS
#error drive.h may only be included on Windows platforms
#endif

#include <filesystem>
#include <list>
#include <windows.h>

namespace shift {
	namespace io {
		class directory;
		/// <directory.h>
		class file;
		/// <file.h>

		// Completely NON-WOKRING on UNIX, https://unix.stackexchange.com/questions/34858/what-is-the-concept-of-drives-in-unix-systems
		class SHIFT_FILESYSTEM_API drive { // Retrieves the system drive
			protected:
				char m_letter = 0x0; // Representing invalid drive
			private:
				drive(void) noexcept; // System drive
			protected:
				drive(char); // Drive from letter
			public:
				/**
				 * Retrieves the drive letter. This will ALWAYS be in upper case.
				 * @return The current drive letter.
				 */
				inline char get_letter(void) const noexcept {
					return this->m_letter;
				}

				/**
				 * Retrieves validity of the current drive object.
				 */
				inline operator bool(void) const noexcept {
					return !this->is_invalid();
				}

				/**
				 * DWORD representation of the drive
				 * @see GetLogicalDrives()
				 */
				inline DWORD dword_val(void) const noexcept {
					if (this->is_invalid())
						return 0x0;
					return (DWORD) (1 << ((this->m_letter) - (char) 'A'));
				}

				/**
				 * Indicates whether the drive is invalid.
				 *
				 * @return Returns true if the underlying drive is invalid, false otherwise.
				 */
				inline bool is_invalid(void) const noexcept {
					return this->m_letter == 0x0;
				}

				/**
				 * Retrieves the logical drives associated with the underlying letter.
				 * If the underlying drive letter does not exist, this function returns an invalid drive.
				 *
				 * @param[in] letter Drive letter
				 * @return Corresponding drive, maybe be @c invalid.
				 */
				static drive get_drive(char letter);

				/**
				 * Retrieves the system drive (Drive in which Windows is installed on)
				 * @return The system drive.
				 */
				static drive get_system_drive(void);

				/**
				 * Retrieves all logical drives on the system.
				 * @return A list of system drives.
				 */
				static std::list<drive> get_drives(void);
			private:
				void m_init(bool sys = false); // Drive initialization
		};
	}
}

template<typename _CharT, typename _Traits>
inline std::basic_ostream<_CharT, _Traits>& operator<<(std::basic_ostream<_CharT, _Traits>& __os, shift::io::drive __drive) {
	return __os << _CharT(__drive.get_letter()) << _CharT(':') << _CharT(std::filesystem::path::preferred_separator);
}

#endif /* SHIFT_FILESYSTEM_DRIVE_H_ */
