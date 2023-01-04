/**
 * @file include/shiftexception.h
 */

#ifndef SHIFT_EXCEPTION_H_
#define SHIFT_EXCEPTION_H_

#include <stdexcept>

namespace shift {
	class shift_exception : public std::runtime_error {
		public:
			explicit shift_exception(const std::string& err = "") _GLIBCXX_TXN_SAFE;
			virtual ~shift_exception(void) _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW;
			shift_exception(shift_exception&&) _GLIBCXX_NOTHROW;
			shift_exception(const shift_exception&) _GLIBCXX_NOTHROW;

			shift_exception& operator=(shift_exception&&) _GLIBCXX_NOTHROW;
			shift_exception& operator=(const shift_exception&) _GLIBCXX_NOTHROW;
	};
}

#endif /* SHIFT_EXCEPTION_H_ */
