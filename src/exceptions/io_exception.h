/**
 * @file include/ioexception.h
 */

#ifndef SHIFT_IO_EXCEPTION_H_
#define SHIFT_IO_EXCEPTION_H_

#include "shiftexception.h"

namespace shift {
	class io_exception : public shift_exception {
		public:
			explicit io_exception(const std::string& err = "I/O Exception occured") _GLIBCXX_TXN_SAFE;
			virtual ~io_exception(void) _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW;
			io_exception(io_exception&&) _GLIBCXX_NOTHROW;
			io_exception(const io_exception&) _GLIBCXX_NOTHROW;

			io_exception& operator=(io_exception&&) _GLIBCXX_NOTHROW;
			io_exception& operator=(const io_exception&) _GLIBCXX_NOTHROW;
	};
}

#endif /* SHIFT_IO_EXCEPTION_H_ */
