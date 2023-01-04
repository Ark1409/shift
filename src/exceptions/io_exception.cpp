/**
 * @file ioexception.cpp
 */
#include "io_exception.h"
namespace shift {
	io_exception::io_exception(const std::string& __msg) _GLIBCXX_TXN_SAFE : shift_exception(__msg) {
	}

	io_exception::~io_exception(void) _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW {
	}

	io_exception::io_exception(io_exception&& except) _GLIBCXX_NOTHROW : shift_exception(std::move(except)) {
	}

	io_exception::io_exception(const io_exception& except) _GLIBCXX_NOTHROW : shift_exception(except) {
	}

	io_exception& io_exception::operator=(io_exception&& except) _GLIBCXX_NOTHROW {
		shift_exception::operator=(std::move(except));
		return *this;
	}

	io_exception& io_exception::operator=(const io_exception& except) _GLIBCXX_NOTHROW {
		shift_exception::operator=(except);
		return *this;
	}

}
