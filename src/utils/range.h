#ifndef SHIFT_UTILS_RANGE_H_
#define SHIFT_UTILS_RANGE_H_ 1

#include <iterator>
#include <concepts>
#include <initializer_list>
#include <optional>
#include <ranges>
#include <memory>

namespace shift::utils {
    /**
     * Represents a range of elements through a sequence of two iterator objects.
     * @tparam Iterator The iterator type
     */
    template<std::input_or_output_iterator Iterator, std::sentinel_for<Iterator> Sentinel = Iterator>
    class range {
    public:
        typedef Iterator iterator_type;
        typedef Sentinel sentinel_type;
        typedef std::iter_value_t<iterator_type> value_type;
        typedef std::iter_reference_t<iterator_type> reference;
        typedef std::remove_reference_t<reference>* pointer;
        typedef std::iter_difference_t<iterator_type> difference_type;
        typedef typename std::iterator_traits<iterator_type>::iterator_category iterator_category;

        range() = default;

        range(iterator_type begin, sentinel_type end) : m_begin(begin), m_end(end) {}

        template<std::ranges::borrowed_range R>
        range(R&& r) : m_begin(std::ranges::begin(r)), m_end(std::ranges::end(r)) {}

        template<std::ranges::borrowed_range R> requires std::ranges::sized_range<R>
        range(R&& r) : m_begin(std::ranges::begin(r)), m_end(std::ranges::end(r)), m_size_cache(std::ranges::size(r)) {}

        template<typename T>
        range(std::initializer_list<T> list) :
            m_begin(std::ranges::begin(list)), m_end(std::ranges::end(list)), m_size_cache(list.size()) {}

        reference operator[](difference_type index) const requires std::random_access_iterator<iterator_type> {
            return *(m_begin + index);
        }

        iterator_type begin() const {
            return m_begin;
        }

        sentinel_type end() const {
            return m_end;
        }

        bool empty() const {
            return m_begin == m_end;
        }

        difference_type size() const {
            return m_size_cache.has_value() ? *m_size_cache : m_size_cache.emplace(std::ranges::distance(m_begin, m_end));
        }

        reference front() const {
            return *m_begin;
        }

        reference back() const requires std::bidirectional_iterator<sentinel_type> {
            return *std::prev(m_end);
        }

        value_type* data() const requires std::contiguous_iterator<Iterator> { return std::to_address(m_begin); }

        const value_type* cdata() const requires std::contiguous_iterator<Iterator> { return std::to_address(m_begin); }

    private:
        iterator_type m_begin{};
        sentinel_type m_end{};
        mutable std::optional<difference_type> m_size_cache{ std::nullopt };
    };

    template<std::ranges::borrowed_range R>
    range(R&&) -> range<std::ranges::iterator_t<R>, std::ranges::sentinel_t<R>>;

    template<std::ranges::borrowed_range R> requires std::ranges::sized_range<R>
    range(R&&) -> range<std::ranges::iterator_t<R>, std::ranges::sentinel_t<R>>;

    template<typename T>
    range(std::initializer_list<T>) -> range<typename std::initializer_list<T>::iterator>;

    template<typename R, typename ValueT>
    concept range_of = std::ranges::range<R> && std::convertible_to<std::ranges::range_value_t<R>, ValueT>;

    template<typename Iterator, typename ValueT>
    concept iter_of = std::convertible_to<std::iter_value_t<Iterator>, ValueT>;

    template<typename T, typename CategoryTag = std::random_access_iterator_tag>
    struct value_iterator {
        typedef T value_type;
        typedef value_type& reference;
        typedef std::remove_reference_t<reference>* pointer;
        typedef std::ptrdiff_t difference_type;
        typedef CategoryTag iterator_category;

    private:
        struct iter_base {
            virtual iter_base& operator++() = 0;

            virtual iter_base& operator++(int) = 0;

            virtual reference operator*() = 0;

            virtual pointer operator->() = 0;
        };

    private:
        std::unique_ptr<iter_base> m_iter;
    };
}

template<std::input_or_output_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
constexpr bool std::ranges::enable_borrowed_range<shift::utils::range<Iterator, Sentinel>> = true;

#endif