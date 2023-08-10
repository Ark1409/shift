#ifndef SHIFT_REVERSE_ITERATOR_H_
#define SHIFT_REVERSE_ITERATOR_H_ 1

#include <type_traits>

namespace shift::utils {
    template<typename Iterator>
    struct reverse_iterator {
        typedef std::remove_reference_t<Iterator> iterator_type;
        typedef typename iterator_type::reference_type reference_type;
        typedef typename iterator_type::pointer_type pointer_type;
    private:
        typedef reverse_iterator<Iterator> reverse_iterator_type;
    public:
        inline explicit reverse_iterator(const iterator_type& it) noexcept(std::is_nothrow_copy_constructible_v<iterator_type>) : m_it(it) {}
        inline explicit reverse_iterator(iterator_type&& it) noexcept(std::is_nothrow_move_constructible_v<iterator_type>) : m_it(std::move(it)) {}

        inline bool operator==(const reverse_iterator_type& other) const { return this->m_it == other.m_it; }
        inline bool operator!=(const reverse_iterator_type& other) const { return !operator==(other); }

        inline bool operator==(iterator_type const& iter) const { auto tmp = m_it; return (--tmp) == iter; }
        inline bool operator!=(iterator_type const& iter) const { return !operator==(iter); }

        inline reference_type operator*() const { auto tmp = m_it; return (--tmp).operator*(); }
        inline pointer_type operator->() const { auto tmp = m_it; return (--tmp).operator->(); }

        inline reverse_iterator_type& operator++() { --m_it; return *this; }
        inline reverse_iterator_type operator++(int) { return reverse_iterator_type(m_it--); }

        inline reverse_iterator_type& operator--() { ++m_it; return *this; }
        inline reverse_iterator_type operator--(int) { return reverse_iterator_type(m_it++); }

        inline operator iterator_type () const { auto tmp = m_it; return (--tmp); }
    private:
        iterator_type m_it;
    };
}

#endif
