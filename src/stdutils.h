/**
 * @file include/stdutils.h
 */

#ifndef SHIFT_STDUTILS_H_
#define SHIFT_STDUTILS_H_ 1

#define instanceof(var, T) (dynamic_cast<const T*>(&(var)) != nullptr)
#define instance_of(var, T) instanceof(var, T)

#define instance_of_object(var, T) instanceof(var, T)
#define instance_of_obj(var, T) instanceof(var, T)
#define instanceof_object(var, T) instanceof(var, T)
#define instanceof_obj(var, T) instanceof(var, T)

#define instanceof_(var, T) instanceof(*var, T)
#define instance_of_(var, T) instanceof_(var, T)

#define instance_of_ptr(var, T) instanceof_(var, T)
#define instanceof_ptr(var, T) instanceof_(var, T)
#define instance_of_pointer(var, T) instanceof_(var, T)
#define instanceof_pointer(var, T) instanceof_(var, T)

#define is_between_in(val,min,max) (((val)>=(min))&&((val)<=(max))) // inclusive
#define is_between_ex(val,min,max) (((val)>(min))&&((val)<(max))) // exclusive
#define shift_min(val, min) ((val) > (min) ? (min) : (val))
#define shift_max(val, max) ((val) < (max) ? (max) : (val))
#define shift_clamp(val, min, max) shift_min(shift_max(val, min), max)

//#define is_whitespace(__str) ((__str)=='\t'||(__str)=='\v'||(__str)==' '||(__str)=='\n'||(__str)=='\r'||(__str)=='\f')
#define is_whitespace(__str) std::isspace(__str)
#define is_whitespace_ext(__str, _CharT) ((__str)==static_cast<_CharT>('\t')||(__str)==static_cast<_CharT>('\v')||(__str)==static_cast<_CharT>(' ')||(__str)==static_cast<_CharT>('\n')||(__str)==static_cast<_CharT>('\r')||(__str)==static_cast<_CharT>('\f'))

#define string_starts_with(__str__, __text__) (std::strcmp(&(__str__)[0], &(__text__)[0]) == 0)

#include <type_traits>
#include <iostream>
#include <list>
#include <vector>
#include <stack>
#include <string>
#include <string_view>
#include <cstring>
#include <stdexcept>
#include <chrono>

#include "shift_config.h"

#define CXX11 201103L // Constant for C++11
#define CXX14 201402L // Constant for C++14
#define CXX17 201703L // Constant for C++17
#define CXX20 (CXX17+1L) // Constant for C++20, standard has not yet defined exact value

#if __cplusplus == CXX11
#define SHIFT_CXX11 1 // Using C++11
#endif

#if __cplusplus == CXX14
#define SHIFT_CXX14 1 // Using C++14
#endif

#if __cplusplus == CXX17
#define SHIFT_CXX17 1 // Using C++17
#endif

#if __cplusplus == CXX20
#define SHIFT_CXX20 1
#endif

#if __cplusplus >= CXX14
#	define CXX14_CONSTEXPR constexpr
#else
#	define CXX14_CONSTEXPR
#endif

#if __cplusplus >= CXX17
#	define CXX17_CONSTEXPR constexpr
#else
#	define CXX17_CONSTEXPR
#endif

#if __cplusplus >= CXX20
#	define CXX20_CONSTEXPR constexpr
#else
#	define CXX20_CONSTEXPR
#endif

#if __cplusplus >= CXX11
#define CXX_CONSTEXPR constexpr
#else
#define CXX_CONSTEXPR
#endif

#if __cplusplus >= CXX11
#define CXX_USE_CONSTEXPR constexpr
#else
#define CXX_USE_CONSTEXPR const
#endif
#define CXX_FORCE_CONSTEXPR CXX_USE_CONSTEXPR

#ifdef SHIFT_DEBUG
#define debug_log(X) std::cout << X << '\n'
#else
#define debug_log(X)
#endif

#ifdef SHIFT_DEBUG
#	define FUNCTION_BENCHMARK_BEGIN debug_log("Starting benchmark for function: " << __func__); typename std::chrono::steady_clock::time_point func_begin = std::chrono::steady_clock::now(); size_t func_bytes_alloced = ::bytes_alloced;
#	define FUNCTION_BENCHMARK_END typename std::chrono::steady_clock::time_point func_end = std::chrono::steady_clock::now(); debug_log("Ended benchmark for function: " << __func__ << "; Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(func_end - func_begin) << "ms" << "; Bytes used: " << (::bytes_alloced-func_bytes_alloced) << " (" << ((double) ((double)::bytes_alloced-(double)func_bytes_alloced) / (double)1024)  << " KiB)");
#else
#	define FUNCTION_BENCHMARK_BEGIN
#	define FUNCTION_BENCHMARK_END
#endif

namespace shift {
	template<bool, bool>
	struct _and_ : public std::false_type {

	};

	template<>
	struct _and_<true, true> : public std::true_type {

	};

	template<bool, bool>
	struct _or_ : public std::true_type {

	};

	template<>
	struct _or_<false, false> : public std::false_type {
	};

	template<bool _B1, bool _B2>
	inline constexpr bool _or_v = _or_<_B1, _B2>::value;

	template<bool _B1, bool _B2>
	inline constexpr bool _and_v = _and_<_B1, _B2>::value;

	template<bool _B1, bool _B2>
	inline constexpr bool _or__v = _or_v<_B1, _B2>;

	template<bool _B1, bool _B2>
	inline constexpr bool _and__v = _and_v<_B1, _B2>;

	template<typename _V>
	[[deprecated("std::get changes list indexing from O(1) -> O(n)")]] _V& get(std::list<_V>& __list, const typename std::list<_V>::size_type index) {
		typename std::list<_V>::size_type __index = 0;

		for (_V& __ret : __list) {
			if (__index == index)
				return __ret;
			__index++;
		}
		{
			throw std::out_of_range("Index (" + std::to_string(index) + ") must be smaller than list size (" + std::to_string(__list.size()) + ")");
		}
	}

	template<typename _V>
	[[deprecated("std::get changes list indexing from O(1) -> O(n)")]] const _V& get(const std::list<_V>& __list,
			const typename std::list<_V>::size_type index) {
		typename std::list<_V>::size_type __index = 0;

		for (const _V& __ret : __list) {
			if (__index == index)
				return __ret;
			__index++;
		}

		{
			using namespace std::string_literals;
			throw std::out_of_range("Index (" + std::to_string(index) + ") must be smaller than list size (" + std::to_string(__list.size()) + ")");
		}
	}
	template<typename _V>
	[[deprecated("std::get changes list indexing from O(1) -> O(n)")]] const _V& get(const std::list<_V>&& __list,
			const typename std::list<_V>::size_type index) = delete;

	template<typename _V>
	std::remove_reference_t<_V> remove(std::list<_V>& __list, const typename std::list<_V>::size_type index) {
		typename std::list<_V>::size_type __index = 0;
		for (typename std::list<_V>::iterator it = __list.begin(); it != __list.end(); it++, __index++) {
			if (__index == index) {
				std::remove_reference_t<_V> ret = std::move(*it);
				__list.erase(it);
				return ret;
			}
		}

		{
			using namespace std::string_literals;
			throw std::out_of_range(
					"Index ("s + std::to_string(index) + ") must be smaller than list size ("s + std::to_string(__list.size()) + ")"s);
		}
	}

	template<typename _CharT>
	inline CXX17_CONSTEXPR size_t strlen(const _CharT* const str) {
		size_t i = 0;
		for (; str[i] != _CharT(0x0); ++i);
		return i;
	}

	template<typename _V>
	std::list<_V> sub_list(const std::list<_V>& __list, const typename std::list<_V>::size_type& index /* inclusive */,
			const typename std::list<_V>::size_type& length) {
		using namespace std::string_literals;
		if (index < 0) {
			throw std::out_of_range("Index ("s + std::to_string(index) + ") must be greater than 0"s);
		}

		if (index > __list.size()) {
			throw std::out_of_range(
					"Index ("s + std::to_string(index) + ") must be smaller than list size ("s + std::to_string(__list.size()) + ")"s);
		}

		if (length < 0) {
			throw std::out_of_range("Length ("s + std::to_string(length) + ") must be greater than 0"s);
		}

		if ((index + length) > __list.size()) {
			throw std::out_of_range(
					"Range from index and length ("s + std::to_string(index) + " and "s + std::to_string(length)
							+ ") must within bounds of list size ("s + std::to_string(__list.size()) + ")"s);
		}

		std::list<_V> sub;

		for (typename std::list<_V>::size_type i = index, i1 = 0; i1 < length; i++, i1++) {
			sub.push_back(shift::get(__list, i));
		}

		return sub;
	}

	template<typename _V>
	std::list<_V> sub_list_index(const std::list<_V>& __list, const typename std::list<_V>::size_type& begin_index /* inclusive */,
			const typename std::list<_V>::size_type& end_index /* inclusive */) {
		using namespace std::string_literals;
		if (begin_index < 0) {
			throw std::out_of_range("Begin index ("s + std::to_string(begin_index) + ") must be greater than 0"s);
		}

		if (end_index < 0) {
			throw std::out_of_range("End index ("s + std::to_string(end_index) + ") must be greater than 0"s);
		}

		if (end_index < begin_index) {
			throw std::out_of_range(
					"Begin index ("s + std::to_string(begin_index) + ") must be smaller than end index ("s + std::to_string(end_index) + ")"s);
		}

		//	if (begin_index == end_index)
		//		return std::list<_V>();

		if ((end_index) >= __list.size()) {
			throw std::out_of_range(
					"End index ("s + std::to_string(end_index) + ") must be smaller than list size than list size ("s + std::to_string(__list.size())
							+ ")"s);
		}

		return sub_list(__list, begin_index, (end_index - begin_index + 1));
	}

	template<typename _V>
	std::list<_V> sub_list(std::list<_V>&& __list, const typename std::list<_V>::size_type& index /* inclusive */,
			const typename std::list<_V>::size_type& length) {
		using namespace std::string_literals;
		if (index < 0) {
			throw std::out_of_range("Index ("s + std::to_string(index) + ") must be greater than 0"s);
		}

		if (index > __list.size()) {
			throw std::out_of_range(
					"Index ("s + std::to_string(index) + ") must be smaller than list size ("s + std::to_string(__list.size()) + ")"s);
		}

		if (length < 0) {
			throw std::out_of_range("Length ("s + std::to_string(length) + ") must be greater than 0"s);
		}

		if ((index + length) > __list.size()) {
			throw std::out_of_range(
					"Range from index and length ("s + std::to_string(index) + " and "s + std::to_string(length)
							+ ") must within bounds of list size ("s + std::to_string(__list.size()) + ")"s);
		}

		std::list<_V> sub;

		for (typename std::list<_V>::size_type i = index, i1 = 0; i1 < length; i++, i1++) {
			sub.push_back(std::move(shift::get(__list, i)));
		}

		return sub;
	}

	template<typename _V>
	std::list<_V> sub_list_index(std::list<_V>&& __list, const typename std::list<_V>::size_type& begin_index /* inclusive */,
			const typename std::list<_V>::size_type& end_index /* inclusive */) {
		using namespace std::string_literals;
		if (begin_index < 0) {
			throw std::out_of_range("Begin index ("s + std::to_string(begin_index) + ") must be greater than 0"s);
		}

		if (end_index < 0) {
			throw std::out_of_range("End index ("s + std::to_string(end_index) + ") must be greater than 0"s);
		}

		if (end_index < begin_index) {
			throw std::out_of_range(
					"Begin index ("s + std::to_string(begin_index) + ") must be smaller than end index ("s + std::to_string(end_index) + ")"s);
		}

		//	if (begin_index == end_index)
		//		return std::list<_V>();

		if ((end_index) >= __list.size()) {
			throw std::out_of_range(
					"End index ("s + std::to_string(end_index) + ") must be smaller than list size than list size ("s + std::to_string(__list.size())
							+ ")"s);
		}

		return sub_list(std::move(__list), begin_index, (end_index - begin_index + 1));
	}

	template<typename _V>
	std::vector<_V> sub_vector(const std::vector<_V>& __vector, typename std::vector<_V>::size_type index /* inclusive */,
			const typename std::vector<_V>::size_type& length) {

		if (index < 0) {
			throw std::out_of_range("Index (" + std::to_string(index) + ") must be greater than 0");
		}

		if (index > __vector.size()) {
			throw std::out_of_range(
					"Index (" + std::to_string(index) + ") must be smaller than vector size (" + std::to_string(__vector.size()) + ")");
		}

		if (length < 0) {
			throw std::out_of_range("Length (" + std::to_string(length) + ") must be greater than 0");
		}

		typename std::vector<_V>::size_type const end_index = index + length;

		if ((end_index) > __vector.size()) {
			throw std::out_of_range(
					"Range from index and length (" + std::to_string(index) + " and " + std::to_string(length)
							+ ") must within bounds of vector size (" + std::to_string(__vector.size()) + ")");
		}

		std::vector<_V> vec;
		vec.reserve(length);

		for (; index != end_index; index++) {
			vec.push_back(__vector[index]);
		}

		return vec;
	}

	template<typename _V>
	std::vector<_V> sub_vector_index(const std::vector<_V>& __vector, const typename std::vector<_V>::size_type& begin_index /* inclusive */,
			const typename std::vector<_V>::size_type& end_index /* inclusive */) {
		if (begin_index < 0) {
			throw std::out_of_range("Begin index (" + std::to_string(begin_index) + ") must be greater than 0");
		}

		if (end_index < 0) {
			throw std::out_of_range("End index (" + std::to_string(end_index) + ") must be greater than 0");
		}

		if (end_index < begin_index) {
			throw std::out_of_range(
					"Begin index (" + std::to_string(begin_index) + ") must be smaller than end index (" + std::to_string(end_index) + ")");
		}

		if ((end_index) >= __vector.size()) {
			throw std::out_of_range(
					"End index (" + std::to_string(end_index) + ") must be smaller than vector size than list size ("
							+ std::to_string(__vector.size()) + ")");
		}

		return sub_vector(__vector, begin_index, (end_index - begin_index + 1));
	}

	template<typename _V>
	std::vector<_V> sub_vector(std::vector<_V>&& __vector, typename std::vector<_V>::size_type index /* inclusive */,
			const typename std::vector<_V>::size_type& length) {

		if (index < 0) {
			throw std::out_of_range("Index (" + std::to_string(index) + ") must be greater than 0");
		}

		if (index > __vector.size()) {
			throw std::out_of_range(
					"Index (" + std::to_string(index) + ") must be smaller than vector size (" + std::to_string(__vector.size()) + ")");
		}

		if (length < 0) {
			throw std::out_of_range("Length (" + std::to_string(length) + ") must be greater than 0");
		}

		typename std::vector<_V>::size_type const end_index = index + length;

		if ((end_index) > __vector.size()) {
			throw std::out_of_range(
					"Range from index and length (" + std::to_string(index) + " and " + std::to_string(length)
							+ ") must within bounds of vector size (" + std::to_string(__vector.size()) + ")");
		}

		std::vector<_V> vec;
		vec.reserve(length);

		for (; index != end_index; index++) {
			vec.push_back(std::move(__vector[index]));
		}

		return vec;
	}

	template<typename _V>
	std::vector<_V> sub_vector_index(std::vector<_V>&& __vector, const typename std::vector<_V>::size_type& begin_index /* inclusive */,
			const typename std::vector<_V>::size_type& end_index /* inclusive */) {
		if (begin_index < 0) {
			throw std::out_of_range("Begin index (" + std::to_string(begin_index) + ") must be greater than 0");
		}

		if (end_index < 0) {
			throw std::out_of_range("End index (" + std::to_string(end_index) + ") must be greater than 0");
		}

		if (end_index < begin_index) {
			throw std::out_of_range(
					"Begin index (" + std::to_string(begin_index) + ") must be smaller than end index (" + std::to_string(end_index) + ")");
		}

		if ((end_index) >= __vector.size()) {
			throw std::out_of_range(
					"End index (" + std::to_string(end_index) + ") must be smaller than vector size than list size ("
							+ std::to_string(__vector.size()) + ")");
		}

		return sub_vector(std::move(__vector), begin_index, (end_index - begin_index + 1));
	}

	template<typename _CharT, typename _Traits, typename _Alloc>
	void replace_all(std::basic_string<_CharT, _Traits, _Alloc>& str, const std::basic_string<_CharT, _Traits, _Alloc>& replace,
			const std::basic_string<_CharT, _Traits, _Alloc>& replacement) {
		// Same inner code...
		// No return statement
		size_t start_pos = 0;
		while ((start_pos = str.find(replace, start_pos)) != std::basic_string<_CharT, _Traits, _Alloc>::npos) {
			str.replace(start_pos, replace.length(), replacement);
			start_pos += replacement.length(); // Handles case where 'to' is a substring of 'from'
		}
		//return str;
	}

	template<typename _CharT, typename _Traits, typename _Alloc>
	inline std::basic_string<_CharT, _Traits, _Alloc> replace_all(const std::basic_string<_CharT, _Traits, _Alloc>& str,
			const std::basic_string<_CharT, _Traits, _Alloc>& replace, const std::basic_string<_CharT, _Traits, _Alloc>& replacement) {
		std::basic_string<_CharT, _Traits, _Alloc> str_new = str;
		replace_all<_CharT, _Traits, _Alloc>(str_new, replace, replacement);
		return str_new;
	}

	template<typename _CharT, typename _Traits, typename _Alloc, typename _Func>
	void replace_all(std::basic_string<_CharT, _Traits, _Alloc>& str, const std::basic_string<_CharT, _Traits, _Alloc>& replace,
			const std::basic_string<_CharT, _Traits, _Alloc>& replacement, _Func on_each) {
		// Same inner code...
		// No return statement
		size_t start_pos = 0;
		while ((start_pos = str.find(replace, start_pos)) != std::basic_string<_CharT, _Traits, _Alloc>::npos) {
			std::basic_string<_CharT, _Traits, _Alloc> __find = str.substr(start_pos, replace.length());
			str.replace(start_pos, replace.length(), replacement);
			start_pos += replacement.length(); // Handles case where 'to' is a substring of 'from'
			on_each(start_pos);
		}
		//return str;
	}

	template<typename _CharT, typename _Traits, typename _Alloc, typename _Func>
	inline std::basic_string<_CharT, _Traits, _Alloc> replace_all(const std::basic_string<_CharT, _Traits, _Alloc>& str,
			const std::basic_string<_CharT, _Traits, _Alloc>& replace, const std::basic_string<_CharT, _Traits, _Alloc>& replacement, _Func on_each) {
		std::basic_string<_CharT, _Traits, _Alloc> str_new = str;
		replace_all<_CharT, _Traits, _Alloc>(str_new, replace, replacement, on_each);
		return str_new;
	}

	template<typename _V>
	bool list_contains(const std::list<_V>& __list, const _V& __val) noexcept {
		for (const _V& __v : __list) {
			if (__v == __val)
				return true;
		}
		return false;
	}

	template<typename _V>
	std::vector<_V> list_to_vector(const std::list<_V>& list) noexcept(std::is_nothrow_copy_constructible<_V>::value) {
		std::vector<_V> vec;
		vec.reserve(list.size());
		for (_V const& val : list) {
			vec.push_back(val);
		}

		return vec;
	}

	template<typename _V>
	std::vector<_V> list_to_vector(std::list<_V>&& list) noexcept(std::is_nothrow_move_constructible<_V>::value) {
		std::vector<_V> vec;
		vec.reserve(list.size());

		for (_V& val : list) {
			vec.push_back(std::move(val));
		}

		return vec;
	}

	template<typename _V>
	std::list<_V> vector_to_list(const std::vector<_V>& vec) noexcept(std::is_nothrow_copy_constructible<_V>::value) {
		std::list<_V> list;

		for (_V const& val : vec) {
			list.push_back(val);
		}

		return list;
	}

	template<typename _V>
	std::list<_V> vector_to_list(std::vector<_V>&& vec) noexcept(std::is_nothrow_move_constructible<_V>::value) {
		std::list<_V> list;

		for (_V& val : vec) {
			list.push_back(std::move(val));
		}

		return list;
	}

	template<typename _V>
	inline std::vector<_V> to_vector(const std::list<_V>& list) noexcept(std::is_nothrow_copy_constructible<_V>::value) {
		return list_to_vector<_V>(list);
	}

	template<typename _V>
	inline std::vector<_V> to_vector(std::list<_V>&& list) noexcept(std::is_nothrow_move_constructible<_V>::value) {
		return list_to_vector<_V>(std::move(list));
	}

	template<typename _V>
	inline std::list<_V> to_list(const std::vector<_V>& list) noexcept(std::is_nothrow_copy_constructible<_V>::value) {
		return vector_to_list<_V>(list);
	}

	template<typename _V>
	inline std::list<_V> to_list(std::vector<_V>&& vec) noexcept(std::is_nothrow_move_constructible<_V>::value) {
		return vector_to_list<_V>(std::move(vec));
	}

	template<typename _CharIn, typename _CharOut, typename _TraitsIn = std::char_traits<_CharIn>, typename _TraitsOut = std::char_traits<_CharOut>,
			typename _AllocIn = std::allocator<_CharIn>, typename _AllocOut = std::allocator<_CharOut>>
	std::basic_string<_CharOut, _TraitsOut, _AllocOut> str_to_str(const std::basic_string<_CharIn, _TraitsIn, _AllocIn>& in) noexcept {
		std::basic_string<_CharOut, _TraitsOut, _AllocOut> out;

		typedef typename std::basic_string<_CharIn, _TraitsIn, _AllocIn>::size_type size_type;
		const size_type len = in.size();
		out.reserve(len);

		for (size_type i = 0; i < len; i++) {
			out.push_back(static_cast<_CharOut>(in[i]));
		}

		return out;
	}

	template<typename _CharIn, typename _CharOut, typename _TraitsOut = std::char_traits<_CharOut>, typename _AllocOut = std::allocator<_CharOut>>
	std::basic_string<_CharOut, _TraitsOut, _AllocOut> str_to_str(const _CharIn* const in) noexcept {
		std::basic_string<_CharOut, _TraitsOut, _AllocOut> out;

		typedef typename std::basic_string<_CharOut, _TraitsOut, _AllocOut>::size_type size_type;
		size_type const len = strlen<_CharIn>(in);
		out.reserve(len);

		for (size_type i = 0; i < len; i++) {
			out.push_back(static_cast<_CharOut>(in[i]));
		}

		return out;
	}

	template<typename _CharT>
	size_t count(const _CharT* const str, const _CharT* const delim) noexcept {
		typedef size_t size_type;

		size_type count = 0;

		const size_type len = strlen<_CharT>(str);
		const size_type delim_len = strlen<_CharT>(str);

		if (delim_len && delim_len <= len) {
			for (size_type i = 0; i < len && (i + delim_len - 1) < len; i++) {
				if (std::memcmp(str, delim, delim_len) == 0) {
					count++;
					i += delim_len - 1;
				}
			}
		}

		return count;
	}

	template<typename _CharT, typename _Traits = std::char_traits<_CharT>>
	std::vector<std::basic_string_view<_CharT, _Traits>> split(const _CharT* const str, const _CharT* const delim) {
		typedef std::basic_string_view<_CharT, _Traits> string_view_type;
		typedef std::vector<string_view_type> vector_type;
		typedef typename std::vector<string_view_type>::size_type size_type;

		vector_type tokens;

		const size_type len = strlen<_CharT>(str);
		const size_type delim_len = strlen<_CharT>(str);

		tokens.reserve(count(str, delim) + 1);

		{
			size_type i = 0, last_pos = 0;
			for (; i < len && (i + delim_len - 1) < len; i++) {
				if (std::memcmp(str, delim, delim_len) == 0) {
					i += delim_len - 1;
					tokens.push_back(string_view_type(&str[last_pos], i - last_pos));
					last_pos = i + 1;
				}
			}

			if (i < len) {
				tokens.push_back(string_view_type(&str[last_pos], len - last_pos));
			}
		}

		return tokens;
	}

	template<typename _CharT, typename _Traits, typename _Alloc>
	inline typename std::basic_string<_CharT, _Traits, _Alloc>::size_type count(const std::basic_string<_CharT, _Traits, _Alloc>& str,
			const std::basic_string<_CharT, _Traits, _Alloc>& delim) {
		return count(str.c_str(), delim.c_str());
	}

	template<typename _CharT, typename _Traits, typename _Alloc>
	inline typename std::basic_string<_CharT, _Traits, _Alloc>::size_type count(const std::basic_string<_CharT, _Traits, _Alloc>& str,
			const std::basic_string_view<_CharT, _Traits> delim) {
		return count(str.c_str(), delim.data());
	}

	template<typename _CharT, typename _Traits, typename _Alloc>
	inline typename std::basic_string_view<_CharT, _Traits>::size_type count(const std::basic_string_view<_CharT, _Traits> str,
			const std::basic_string<_CharT, _Traits, _Alloc>& delim) {
		return count(str.data(), delim.c_str());
	}

	template<typename _CharT, typename _Traits>
	inline typename std::basic_string_view<_CharT, _Traits>::size_type count(const std::basic_string_view<_CharT, _Traits> str,
			const std::basic_string_view<_CharT, _Traits> delim) {
		return count(str.data(), delim.data());
	}

	template<typename _CharT, typename _Traits, typename _Alloc>
	inline std::vector<std::basic_string_view<_CharT, _Traits>> split(const std::basic_string<_CharT, _Traits, _Alloc>& str,
			const std::basic_string<_CharT, _Traits, _Alloc>& delim) {
		return split<_CharT, _Traits>(str.c_str(), delim.c_str());
	}

	template<typename _CharT, typename _Traits, typename _Alloc>
	inline std::vector<std::basic_string_view<_CharT, _Traits>> split(const std::basic_string<_CharT, _Traits, _Alloc>& str,
			const std::basic_string_view<_CharT, _Traits> delim) {
		return split<_CharT, _Traits>(str.c_str(), delim.data());
	}

	template<typename _CharT, typename _Traits, typename _Alloc>
	inline std::vector<std::basic_string_view<_CharT, _Traits>> split(const std::basic_string_view<_CharT, _Traits> str,
			const std::basic_string<_CharT, _Traits, _Alloc>& delim) {
		return split<_CharT, _Traits>(str.data(), delim.c_str());
	}

	template<typename _CharT, typename _Traits>
	inline std::vector<std::basic_string_view<_CharT, _Traits>> split(const std::basic_string_view<_CharT, _Traits> str,
			const std::basic_string_view<_CharT, _Traits> delim) {
		return split<_CharT, _Traits>(str.data(), delim.data());
	}

	template<typename T>
	inline std::stack<T>& clear(std::stack<T>& stack) {
		while (!stack.empty())
			stack.pop();
		return stack;
	}

	[[noreturn]] SHIFT_API extern void exit(int status = 0x1);
//#ifdef SHIFT_DEBUG
//extern void print_stats() noexcept;
//#endif
}

template<typename _CharT, typename _Traits, typename _Rep, typename _Period>
inline std::basic_ostream<_CharT, _Traits>& operator<<(std::basic_ostream<_CharT, _Traits>& __os, const std::chrono::duration<_Rep, _Period> dur) {
	return __os << dur.count();
}

template<typename _CharT, typename _Traits, typename _V>
std::basic_ostream<_CharT, _Traits>& operator<<(std::basic_ostream<_CharT, _Traits>& __os, const std::vector<_V>& __vector) {
	typedef typename std::vector<_V>::size_type size_type;
	__os << _CharT('[');

	for (size_type i = 0; i < __vector.size(); i++) {
		if (i > 0)
			__os << _CharT(',') << _CharT(' ');
		__os << __vector[i];
	}
	__os << _CharT(']');
	return __os;
}

template<typename _CharT, typename _Traits, typename _V>
std::basic_ostream<_CharT, _Traits>& operator<<(std::basic_ostream<_CharT, _Traits>& __os, const std::list<_V>& __list) {
	typedef typename std::list<_V>::size_type size_type;
	__os << _CharT('[');
	size_type i = 0;
	for (_V const& __v : __list) {
		if (i > 0)
			__os << _CharT(',') << _CharT(' ');
		__os << __v;
		i++;
	}
	__os << _CharT(']');
	return __os;
}

template<typename _V>
inline typename std::list<_V>::iterator operator+(typename std::list<_V>::iterator it, typename std::list<_V>::size_type const count) noexcept {
	for (typename std::list<_V>::size_type i = 0; i < count; i++, ++it);
	return it;
}

template<typename _V>
inline typename std::list<_V>::const_iterator operator+(typename std::list<_V>::const_iterator it, typename std::list<_V>::size_type const count)
		noexcept {
	for (typename std::list<_V>::size_type i = 0; i < count; i++, ++it);
	return it;
}

template<typename _V>
inline typename std::list<_V>::iterator operator-(typename std::list<_V>::iterator it, typename std::list<_V>::size_type count) noexcept {
	for (; count > 0; --count, --it);
	return it;
}

template<typename _V>
inline typename std::list<_V>::const_iterator operator-(typename std::list<_V>::const_iterator it, typename std::list<_V>::size_type count) noexcept {
	for (; count > 0; --count, --it);
	return it;
}

template<typename _V>
std::make_signed_t<typename std::list<_V>::size_type> operator-(typename std::list<_V>::iterator it1, typename std::list<_V>::iterator it2) {
	std::make_signed_t<typename std::list<_V>::size_type> distance = 0;
	{
		typename std::list<_V>::iterator it_loop = it2;
		for (; it_loop != it1 && it_loop._M_node; ++it_loop, distance++);

		if (it_loop != it1) {
			it_loop = it2;
			distance = 0;
			for (; it_loop != it1 && it_loop._M_node; --it_loop, distance--);

			if (it_loop != it1) {
				throw std::invalid_argument("Iterators must be of the same list!");
			}
		}
	}

	return distance;
}

template<typename _V>
std::make_signed_t<typename std::list<_V>::size_type> operator-(typename std::list<_V>::const_iterator it1,
		typename std::list<_V>::const_iterator it2) {
	std::make_signed_t<typename std::list<_V>::size_type> distance = 0;
	{
		typename std::list<_V>::const_iterator it_loop = it2;
		for (; it_loop != it1 && it_loop._M_node; ++it_loop, distance++);

		if (it_loop != it1) {
			it_loop = it2;
			distance = 0;
			for (; it_loop != it1 && it_loop._M_node; --it_loop, distance--);

			if (it_loop != it1) {
				throw std::invalid_argument("Iterators must be of the same list!");
			}
		}
	}

	return distance;
}

#endif /* SHIFT_STDUTILS_H_ */
