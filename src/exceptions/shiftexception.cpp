/**
 * @file shiftexception.cpp
 */
#include "shiftexception.h"

namespace shift {
	shift_exception::shift_exception(const std::string& __msg) _GLIBCXX_TXN_SAFE : std::runtime_error(__msg) {
	}

	shift_exception::~shift_exception(void) _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW {
	}

	shift_exception::shift_exception(shift_exception&& except) _GLIBCXX_NOTHROW : std::runtime_error(std::move(except)) {
	}

	shift_exception::shift_exception(const shift_exception& except) _GLIBCXX_NOTHROW : std::runtime_error(except) {
	}

	shift_exception& shift_exception::operator=(shift_exception&& except) _GLIBCXX_NOTHROW {
		std::runtime_error::operator=(std::move(except));
		return *this;
	}

	shift_exception& shift_exception::operator=(const shift_exception& except) _GLIBCXX_NOTHROW {
		std::runtime_error::operator=(except);
		return *this;
	}
}
