#ifndef SHIFT_UTILS_RANGE_H_
#define SHIFT_UTILS_RANGE_H_ 1

#include <iterator>
#include <concepts>
#include <initializer_list>
#include <optional>

namespace shift::utils {
    template<typename InIterator, typename OutIterator, typename Transformer>
    struct iterator_transformer {
        typedef std::remove_cvref_t<InIterator> in_iterator_type;
        typedef std::remove_cvref_t<OutIterator> iterator_type;
        typedef std::remove_cvref_t<Transformer> transformer_type;
        typedef std::iter_value_t<iterator_type> value_type;
        typedef std::iter_reference_t<iterator_type> reference;
        typedef std::iter_difference_t<iterator_type> difference_type;
        typedef typename std::iterator_traits<iterator_type>::iterator_category iterator_category;

        iterator_transformer(iterator_type it, transformer_type transformer = transformer_type()) : m_it(m_transformer.from(it)), m_transformer(transformer) {}
        iterator_transformer(in_iterator_type it, transformer_type transformer = transformer_type()) : m_it(it), m_transformer(transformer) {}

        decltype(auto) operator*() const requires std::indirectly_readable<iterator_type> {
            return *m_transformer.to(m_it);
        }

        iterator_transformer& operator++() requires std::forward_iterator<iterator_type> {
            auto it = m_transformer.to(m_it);
            m_it = m_transformer.from(++it);
            return *this;
        }

        iterator_transformer operator++(int) requires std::forward_iterator<iterator_type> {
            iterator_transformer it = *this;
            ++(*this);
            return it;
        }

        iterator_transformer& operator--() requires std::bidirectional_iterator<iterator_type> {
            auto it = m_transformer.to(m_it);
            m_it = m_transformer.from(--it);
            return *this;
        }

        iterator_transformer operator--(int) requires std::bidirectional_iterator<iterator_type> {
            iterator_transformer it = *this;
            --(*this);
            return it;
        }

        iterator_transformer& operator+=(difference_type diff) requires std::random_access_iterator<iterator_type> {
            auto it = m_transformer.to(m_it);
            m_it = m_transformer.from(it += diff);
            return *this;
        }

        iterator_transformer operator+(difference_type diff) requires std::random_access_iterator<iterator_type> {
            iterator_transformer it = *this;
            it += diff;
            return it;
        }

        iterator_transformer& operator-=(difference_type diff) requires std::random_access_iterator<iterator_type> {
            auto it = m_transformer.to(m_it);
            m_it = m_transformer.from(it -= diff);
            return *this;
        }

        iterator_transformer operator-(difference_type diff) requires std::random_access_iterator<iterator_type> {
            iterator_transformer it = *this;
            it -= diff;
            return it;
        }

        bool operator==(const iterator_transformer& other) const requires std::equality_comparable<iterator_type> {
            auto it = m_transformer.to(m_it);
            auto it_other = m_transformer.to(other.m_it);
            return it == it_other;
        }

        bool operator!=(const iterator_transformer& other) const requires std::equality_comparable<iterator_type> {
            auto it = m_transformer.to(m_it);
            auto it_other = m_transformer.to(other.m_it);
            return it != it_other;
        }

        bool operator<(const iterator_transformer& other) const requires std::random_access_iterator<iterator_type> {
            auto it = m_transformer.to(m_it);
            auto it_other = m_transformer.to(other.m_it);
            return it < it_other;
        }

        bool operator<=(const iterator_transformer& other) const requires std::random_access_iterator<iterator_type> {
            auto it = m_transformer.to(m_it);
            auto it_other = m_transformer.to(other.m_it);
            return it <= it_other;
        }

        bool operator>(const iterator_transformer& other) const requires std::random_access_iterator<iterator_type> {
            auto it = m_transformer.to(m_it);
            auto it_other = m_transformer.to(other.m_it);
            return it > it_other;
        }
        bool operator>=(const iterator_transformer& other) const requires std::random_access_iterator<iterator_type> {
            auto it = m_transformer.to(m_it);
            auto it_other = m_transformer.to(other.m_it);
            return it >= it_other;
        }
    private:
        in_iterator_type m_it;
        transformer_type m_transformer = transformer_type();
    };


    template<typename Iterator>
    class range {
    public:
        typedef Iterator iterator_type;
        typedef std::iter_value_t<iterator_type> value_type;
        typedef std::iter_reference_t<iterator_type> reference;
        typedef std::iter_difference_t<iterator_type> difference_type;
        typedef typename std::iterator_traits<iterator_type>::iterator_category iterator_category;

        range(iterator_type begin, iterator_type end) : m_begin(begin), m_end(end) {}

        template<typename C>
        range(C& c) : m_begin(std::begin(c)), m_end(std::end(c)), m_size_cache(c.size()) {}

        template<typename C>
        range(const C& c) : m_begin(std::begin(c)), m_end(std::end(c)), m_size_cache(c.size()) {}

        template<typename T, std::size_t N>
        range(T(&a)[N]) : m_begin(std::begin(a)), m_end(std::end(a)), m_size_cache(N) {}

        template<typename T>
        range(std::initializer_list<T> list) : m_begin(std::begin(list)), m_end(std::end(list)), m_size_cache(list.size()) {}

        reference operator[](difference_type index) const requires std::random_access_iterator<iterator_type> {
            return *(m_begin + index);
        }

        iterator_type begin() const {
            return m_begin;
        }

        iterator_type end() const {
            return m_end;
        }

        bool empty() const {
            return m_begin == m_end;
        }

        difference_type size() const {
            return m_size_cache.has_value() ? *m_size_cache : m_size_cache.emplace(std::distance(m_begin, m_end));
        }

        reference front() const {
            return *m_begin;
        }

        reference back() const requires std::bidirectional_iterator<iterator_type> {
            return *std::prev(m_end);
        }

    private:
        iterator_type m_begin, m_end;
        mutable std::optional<difference_type> m_size_cache{ std::nullopt };
    };

    template<typename C>
    range(C&) -> range<typename C::iterator>;

    template<typename C>
    range(const C&) -> range<typename C::const_iterator>;

    template<typename T, std::size_t N>
    range(T(&a)[N]) -> range<T*>;

    template<typename T>
    range(std::initializer_list<T>) -> range<typename std::initializer_list<T>::iterator>;

    template<typename T, typename ValueT>
    concept range_of = std::convertible_to<typename range<T>::value_type, ValueT>;
}

#endif