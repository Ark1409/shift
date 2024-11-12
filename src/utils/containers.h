#ifndef SHIFT_UTILS_CONTAINERS_H_
#define SHIFT_UTILS_CONTAINERS_H_ 1

#include <ranges>
#include <ostream>
#include <stack>
#include <list>
#include <vector>
#include <stdexcept>

namespace shift::utils {
    template<typename T, typename Seq>
    inline std::stack<T, Seq>& clear_stack(std::stack<T, Seq>& stack) {
        std::stack<T, Seq>().swap(stack);
        return stack;
    }

    template<typename T, typename Seq>
    inline std::stack<T, Seq>&
    pop_stack(std::stack<T, Seq>& stack, typename std::stack<T, Seq>::size_type count = typename std::stack<T, Seq>::size_type(1)) {
        if (count >= stack.size()) { return clear_stack(stack); }

        for (; count > 0; count--) { stack.pop(); }

        return stack;
    }

    template<std::ranges::input_range R> requires requires(R r) {
    r.
    erase(std::ranges::iterator_t<R>{}
    );
}

auto erase(R&& r, const std::ranges::range_difference_t<R> index) {
    if (index < 0) {
        throw std::out_of_range(std::to_string(index) + " is not in range [0, " + std::to_string(std::ranges::distance(r)) + ")");
    }

    if constexpr (std::ranges::random_access_range<R>) {
        auto size = std::ranges::distance(r);
        if (index >= size) {
            throw std::out_of_range(std::to_string(index) + " is not in range [0, " + std::to_string(size) + ")");
        }
        auto it = std::ranges::begin(r);
        std::advance(it, index);

        auto ret = std::move(*it);
        r.erase(it);
        return ret;
    } else {
        auto it = std::ranges::begin(r);
        std::ranges::range_difference_t<R> loop_count{ 0 };
        auto i{ index };
        for (; i != 0 && it != std::ranges::end(r); --i, ++it, ++loop_count) {}

        if (i != 0) {
            throw std::out_of_range(std::to_string(index) + " is not in range [0, " + std::to_string(loop_count) + ")");
        }

        auto ret = std::move(*it);
        r.erase(it);
        return ret;
    }
}

}

/* std namespace injection */
namespace std {
    template<typename CharT, typename Traits, std::ranges::input_range R>
    std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& _os, R&& r) {
        _os << _os.widen('[');

        for (bool past_first = false; auto& v : r) {
            if (past_first) { _os << _os.widen(',') << _os.widen(' '); }
            _os << v;
            past_first = true;
        }
        _os << _os.widen(']');
        return _os;
    }
}

#endif //SHIFT_UTILS_CONTAINERS_H_
