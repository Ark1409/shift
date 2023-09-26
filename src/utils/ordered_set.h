#ifndef SHIFT_ORDERED_SET_H_
#define SHIFT_ORDERED_SET_H_ 1

#include "ordered_set_iterator.h"
#include "reverse_iterator.h"

#include <list>
#include <unordered_set>
#include <unordered_map>

namespace shift::utils {
    /**
     * @brief ordered_set is an ordered container that contains a set of unique objects of type V. Search, insertion,
     * and removal of elements have average constant-time complexity. Internally, this uses std::unordered_map, std::unordered_set,
     * and std::list in order to provide (average) constant-time complexity.
     *
     * Container elements may not be modified (even by non const iterators) since modification could change an element's hash and corrupt the container.
     *
     * @tparam V Type of mapped objects.
     * @tparam Hash Hashing function object type, defaults to std::hash<V>.
     * @tparam Pred Predicate function object type, defaults to std::equal_to<V>
     * @tparam Alloc Allocator type, defaults to std::allocator<V>.
     */
    template<typename V, typename Hash = std::hash<V>, typename Pred = std::equal_to<V>, typename Alloc = std::allocator<V>>
    class ordered_set {
    private:
        typedef std::unordered_set<V, Hash, Pred, Alloc> unordered_set_type;
    public:
        typedef typename unordered_set_type::key_type               key_type;
        typedef typename unordered_set_type::value_type	            value_type;
        typedef typename unordered_set_type::hasher                 hasher;
        typedef typename unordered_set_type::key_equal              key_equal;
        typedef typename unordered_set_type::allocator_type         allocator_type;

        typedef typename unordered_set_type::pointer                pointer;
        typedef typename unordered_set_type::const_pointer          const_pointer;
        typedef typename unordered_set_type::reference              reference;
        typedef typename unordered_set_type::const_reference        const_reference;
        typedef typename unordered_set_type::size_type              size_type;
        typedef typename unordered_set_type::difference_type        difference_type;

        typedef ordered_set_iterator<V, Hash, Pred, Alloc>          iterator;
        typedef ordered_set_iterator<V const, Hash, Pred, Alloc>    const_iterator;

        typedef utils::reverse_iterator<iterator>                   reverse_iterator;
        typedef utils::reverse_iterator<const_iterator>             const_reverse_iterator;
    public:
        /**
         * @brief Default constructor. Constructs an empty container with a default-constructed allocator.
         */
        ordered_set() = default;

        /**
         * @brief Copy constructor. Constructs the container with the copy of the contents of other.
         * @param other another container to be used as source to initialize the elements of the container with
         */
        ordered_set(const ordered_set& other) {
            for (value_type const* p : other.m_order) {
                // Standard states element-wise moved should be performed in normal case,
                // Creating an allowance for the assumption that p musn't be const in impl
                auto [it, unused] = this->m_set.insert(*p);
                this->m_order.push_back(it.operator->());
                this->m_lookup[it.operator->()] = --this->m_order.cend();
            }
        }

        /**
         * @brief Move constructor. Constructs the container with the contents of other using move semantics.
         * @param other another container to be used as source to initialize the elements of the container with
         */
        ordered_set(ordered_set&& other) {
            // Moving may invalidate iterators, see https://stackoverflow.com/a/11022447 and https://en.cppreference.com/w/cpp/container/unordered_set/operator%3D
            if constexpr ((!std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value && this->m_set.get_allocator() != other.m_set.get_allocator())
                || (!std::allocator_traits<typename std::list<value_type const*>::allocator_type>::propagate_on_container_move_assignment::value && this->m_order.get_allocator() != other.m_order.get_allocator())) {
                // this->m_set or this->m_order will alloc new memory when moving

                for (value_type const* p : other.m_order) {
                    // Standard states element-wise moved should be performed in normal case,
                    // Creating an allowance for the assumption that p musn't be const in impl
                    auto [it, unused] = this->m_set.insert(std::move(*const_cast<value_type*>(p)));
                    this->m_order.push_back(it.operator->());
                    this->m_lookup[it.operator->()] = --this->m_order.cend();
                }

                other.m_order.clear();
                other.m_set.clear();
                other.m_lookup.clear();
            } else {
                // this->m_set and this->m_order both will not alloc new memory when moving
                this->m_set = std::move(other.m_set);
                this->m_order = std::move(other.m_order);
                this->m_lookup = std::move(other.m_lookup);
            }
        }

        /**
         * @brief Constructs the container with the contents of the range [first, last). Sets max_load_factor() to 1.0.
         * If multiple elements in the range have keys that compare equivalent, the last one is kept.
         * @tparam InputIt must meet the requirements of LegacyInputIterator.
         * @param first the range to copy the elements from
         * @param last the range to copy the elements from
         */
        template< class InputIt >
        inline ordered_set(InputIt first, InputIt last) {
            for (; first != last; ++first) {
                push_back(std::move(*first));
            }
        }

        /**
         * @brief Constructs the container with the contents of the initializer list init, same as ordered_set(init.begin(), init.end()).
         * If multiple elements in the range have keys that compare equivalent, the last one is kept.
         * @param init initializer list to initialize the elements of the container with
         */
        inline ordered_set(std::initializer_list<value_type> init) : ordered_set(init.begin(), init.end()) { }

        /**
         * @brief Copy assignment operator. Replaces the contents with a copy of the contents of other.
         * @param other another container to use as data source
         * @return @c *this
         */
        ordered_set& operator=(const ordered_set& other) {
            this->m_order.clear();
            this->m_set.clear();
            this->m_lookup.clear();

            for (value_type const* p : other.m_order) {
                // Standard states element-wise moved should be performed in normal case,
                // Creating an allowance for the assumption that p musn't be const in impl
                auto [it, unused] = this->m_set.insert(*p);
                this->m_order.push_back(it.operator->());
                this->m_lookup[it.operator->()] = --this->m_order.cend();
            }

            return *this;
        }

        /**
         * @brief Move assignment operator. Replaces the contents with those of other using move semantics (i.e. the data in other is moved from other into this container).
         * other is in a valid but unspecified state afterwards.
         * @param other another container to use as data source
         * @return *this
         */
        ordered_set& operator=(ordered_set&& other) {
            // Moving may invalidate iterators, see https://stackoverflow.com/a/11022447 and https://en.cppreference.com/w/cpp/container/unordered_set/operator%3D
            if constexpr ((!std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value && this->m_set.get_allocator() != other.m_set.get_allocator())
                || (!std::allocator_traits<typename std::list<value_type const*>::allocator_type>::propagate_on_container_move_assignment::value && this->m_order.get_allocator() != other.m_order.get_allocator())) {
                // this->m_set or this->m_order will alloc new memory when moving
                this->m_order.clear();
                this->m_set.clear();
                this->m_lookup.clear();

                for (value_type const* p : other.m_order) {
                    // Standard states element-wise moved should be performed in normal case,
                    // Creating an allowance for the assumption that p musn't be const in impl
                    auto [it, unused] = this->m_set.insert(std::move(*const_cast<value_type*>(p)));
                    this->m_order.push_back(it.operator->());
                    this->m_lookup[it.operator->()] = --this->m_order.cend();
                }

                other.m_order.clear();
                other.m_set.clear();
                other.m_lookup.clear();
            } else {
                // this->m_set and this->m_order both will not alloc new memory when moving
                this->m_set = std::move(other.m_set);
                this->m_order = std::move(other.m_order);
                this->m_lookup = std::move(other.m_lookup);
            }

            return *this;
        }

        /**
         * @brief Checks if the container has no elements, i.e. whether begin() == end().
         * @return true if the container is empty, false otherwise
         */
        inline bool empty() const noexcept { return m_set.empty(); }

        /**
         * @brief Returns the number of elements in the container, i.e. std::distance(begin(), end()).
         * @return The number of elements in the container.
         */
        inline size_type size() const noexcept { return m_set.size(); }

        /**
         * @brief Returns the maximum number of elements the container is able to hold due to system or library implementation limitations, i.e. std::distance(begin(), end()) for the largest container.
         * @return Maximum number of elements.
         */
        inline size_type max_size() const noexcept { return std::min(m_order.max_size(), std::min(m_set.max_size(), m_lookup.max_size())); }

        /**
         * @brief Erases all elements from the container. After this call, size() returns zero.
         * Invalidates any references, pointers, or iterators referring to contained elements. May also invalidate past-the-end iterators.
         */
        inline void clear() noexcept { m_order.clear(); m_set.clear(); m_lookup.clear(); }

        /**
         * @brief Prepends the given element value to the beginning of the container. No iterators or references are invalidated.
         * @param value the value of the element to prepend
         */
        inline void push_front(const value_type& value) { insert(cbegin(), value); }

        /**
         * @brief Prepends the given element value to the beginning of the container. No iterators or references are invalidated.
         * @param value the value of the element to prepend
         */
        inline void push_front(value_type&& value) { insert(cbegin(), std::move(value)); }

        /**
         * @brief Appends the given element value to the end of the container. No iterators or references are invalidated.
         * @param value the value of the element to append
         */
        inline void push_back(const value_type& value) { insert(cend(), value); }

        /**
         * @brief Appends the given element value to the end of the container. No iterators or references are invalidated.
         * @param value the value of the element to append
         */
        inline void push_back(value_type&& value) { insert(cend(), std::move(value)); }

        /**
         * @brief Removes the first element of the container. Calling pop_front on an empty container is a no-op.
         * References and iterators to the erased element are invalidated.
         */
        inline void pop_front() { if (size() > 0) { erase(m_lookup[m_order.front()]); } }

        /**
         * @brief Removes the last element of the container. Calling pop_back on an empty container is a no-op.
         * References and iterators to the erased element are invalidated.
         */
        inline void pop_back() { if (size() > 0) { erase(m_lookup[m_order.back()]); } }

        /**
         * @brief Attemps to append the given element to the container.
         * @return A std::pair<iterator, bool>, of which the first element is an iterator that points
         *           to the possibly inserted element, and the second is a bool
         *           that is true if the element was actually inserted.
         */
        inline std::pair<iterator, bool> push(const value_type& value) {
            auto it = find(value);
            return it == end() ? std::pair<iterator, bool>{insert(cend(), value), true} : std::pair<iterator, bool>{ it, false };
        }

        /**
         * @brief Attemps to append the given element to the container.
         * @return A std::pair<iterator, bool>, of which the first element is an iterator that points
         *           to the possibly inserted element, and the second is a bool
         *           that is true if the element was actually inserted.
         */
        inline std::pair<iterator, bool> push(value_type&& value) {
            auto it = find(value);
            return it == end() ? std::pair<iterator, bool>{insert(cend(), std::move(value)), true} : std::pair<iterator, bool>{ it, false };
        }

        /**
         *  @brief Builds and inserts an element into the container.
         *  @param args  Arguments used to generate an element.
         *  @return A std::pair<iterator, bool>, of which the first element is an iterator that points
         *           to the possibly inserted element, and the second is a bool
         *           that is true if the element was actually inserted.
         */
        template<typename... Args>
        inline std::pair<iterator, bool> emplace(Args&&... args) { return push(value_type(std::forward<Args>(args)...)); }

        /**
         * @brief Inserts element(s) into the container. If the container doesn't already contain an element with an equivalent key,
         * the element is inserted at the desired position. If the element already exists within the container, it is simply relocated.
         * @param pos iterator before which the content will be inserted (pos may be the end() iterator)
         * @param value element value to insert
         * @return Returns an iterator to the inserted element
         */
        iterator insert(const_iterator pos, const value_type& value) {
            auto c = m_set.find(value);
            if (c == m_set.end()) {
                auto r = m_set.insert(value);
                auto i = m_order.insert(pos.m_order_it, r.first.operator->());
                m_lookup[r.first.operator->()] = i;
                return iterator(i);
            } else {
                auto f = m_lookup.find(c.operator->());
                typename std::list<value_type const*>::const_iterator p_it = f->second;
                auto i = m_order.insert(pos.m_order_it, (*p_it));
                f->second = i;
                m_order.erase(p_it);
                return iterator(i);
            }
        }

        /**
         * @brief Inserts element(s) into the container. If the container doesn't already contain an element with an equivalent key,
         * the element is inserted at the desired position. If the element already exists within the container, it is simply relocated.
         * @param pos iterator before which the content will be inserted (pos may be the end() iterator)
         * @param value element value to insert
         * @return Returns an iterator to the inserted element
         */
        iterator insert(const_iterator pos, value_type&& value) {
            auto c = m_set.find(value);
            if (c == m_set.end()) {
                auto r = m_set.insert(std::move(value));
                auto i = m_order.insert(pos.m_order_it, r.first.operator->());
                m_lookup[r.first.operator->()] = i;
                return iterator(i);
            } else {
                auto f = m_lookup.find(c.operator->());
                typename std::list<value_type const*>::const_iterator p_it = f->second;
                auto i = m_order.insert(pos.m_order_it, (*p_it));
                f->second = i;
                m_order.erase(p_it);
                return iterator(i);
            }
        }

        /**
         * @brief Erases the specified elements from the container.
         * @param pos iterator to the element to remove
         * @return Iterator following the last removed element.
         */
        inline iterator erase(const_iterator pos) {
            m_set.erase(*pos);
            m_lookup.erase(*pos.m_order_it);
            auto i = m_order.erase(pos.m_order_it);
            return iterator(i);
        }

        /**
         * @brief Erases the specified elements from the container.
         * @param pos iterator to the element to remove
         * @return Iterator following the last removed element.
         */
        inline iterator remove(const_iterator pos) { return erase(pos); }

        /**
         * @brief Erases the specified elements from the container.
         * @param key key value of the elements to remove
         * @return Number of elements removed (0 or 1).
         */
        inline size_type erase(const key_type& key) {
            auto f = m_set.find(key);
            if (f == m_set.end()) return 0;
            erase(const_iterator(m_lookup[f.operator->()]));
            return 1;
        }

        /**
         * @brief Erases the specified elements from the container.
         * @param key key value of the elements to remove
         * @return Number of elements removed (0 or 1).
         */
        inline size_type remove(const key_type& key) { return erase(key); }

        /**
         * @brief Exchanges the contents of the container with those of other. Does not invoke any move, copy, or swap operations on individual elements. All iterators and references remain valid. The end() iterator is invalidated.
         * @param other container to exchange the contents with
         */
        inline void swap(ordered_set& other) noexcept(noexcept(std::unordered_set<V, Hash, Pred, Alloc>::swap)) { m_order.swap(other.m_order); m_set.swap(other.m_set); m_lookup.swap(other.m_lookup); }

        /**
         * @brief Sorts the elements in ascending order. The order of equal elements is preserved. The first version uses operator< to compare the elements,
         * the second version uses the given comparison function comp. If an exception is thrown, the order of elements in *this is unspecified.
         */
        inline void sort() { return sort(std::less<value_type>()); }

        /**
         * @brief Sorts the elements in ascending order. The order of equal elements is preserved. The first version uses operator< to compare the elements,
         * the second version uses the given comparison function comp. If an exception is thrown, the order of elements in *this is unspecified.
         * @tparam Compare bool cmp(value_type const& a, value_type const& b);
         * @param comp comparison function object (i.e. an object that satisfies the requirements of Compare) which returns ​true if the first argument is less than (i.e. is ordered before) the second.
         */
        template<class Compare>
        inline void sort(Compare comp) { m_order.sort([&comp](value_type* const a, value_type* const b) { return comp(*a, *b); }); }

        /**
         * @brief Reverses the order of the elements in the container. No references or iterators become invalidated.
         */
        inline void reverse() noexcept { m_order.reverse(); }

        /**
         * @brief Finds an element with key equivalent to key. This function has constant complexity on average, worst case linear in the size of the container.
         * @param key key value of the element to search for
         * @return Iterator to an element with key equivalent to key. If no such element is found, past-the-end (see end()) iterator is returned.
         */
        inline iterator find(const key_type& key) {
            auto look = m_set.find(key);
            if (look == m_set.end()) { return end(); }
            auto f = m_lookup.find(look.operator->());
            return iterator(f->second);
        }

        /**
         * @brief Finds an element with key equivalent to key. This function has constant complexity on average, worst case linear in the size of the container.
         * @param key key value of the element to search for
         * @return Iterator to an element with key equivalent to key. If no such element is found, past-the-end (see end()) iterator is returned.
         */
        inline const_iterator find(const key_type& key) const {
            auto look = m_set.find(key);
            if (look == m_set.end()) { return end(); }
            auto f = m_lookup.find(look.operator->());
            return const_iterator(f->second);
        }

        /**
         * @brief Checks if there is an element with key equivalent to key in the container.
         * @param key key value of the element to search for
         * @return true if there is such an element, otherwise false.
         */
        inline bool contains(const key_type& key) const { return m_set.find(key) != m_set.end(); }

        /**
         * @brief Returns a reference to the last element in the container. Calling back on an empty container causes undefined behavior.
         * @return Reference to the last element.
         */
        inline value_type const& back() const noexcept { return *m_order.back(); }

        /**
         * @brief Returns a reference to the first element in the container. Calling front on an empty container causes undefined behavior.
         * @return Reference to the first element
         */
        inline value_type const& front() const noexcept { return *m_order.front(); }

        /**
         * @brief Returns an iterator to the first element of the unordered_set. If the unordered_set is empty, the returned iterator will be equal to end().
         * @return Iterator to the first element.
         */
        inline iterator begin() noexcept { return iterator(m_order.begin()); }

        /**
         * @brief Returns an iterator to the first element of the unordered_set. If the unordered_set is empty, the returned iterator will be equal to end().
         * @return Iterator to the first element.
         */
        inline const_iterator begin() const noexcept { return const_iterator(m_order.begin()); }

        /**
         * @brief Returns an iterator to the first element of the unordered_set. If the unordered_set is empty, the returned iterator will be equal to end().
         * @return Iterator to the first element.
         */
        inline const_iterator cbegin() const noexcept { return const_iterator(m_order.cbegin()); }

        /**
         * @brief Returns an iterator to the element following the last element of the unordered_set.
         * This element acts as a placeholder; attempting to access it results in undefined behavior.
         * @return Iterator to the element following the last element.
         */
        inline iterator end() noexcept { return iterator(m_order.end()); }

        /**
         * @brief Returns an iterator to the element following the last element of the unordered_set.
         * This element acts as a placeholder; attempting to access it results in undefined behavior.
         * @return Iterator to the element following the last element.
         */
        inline const_iterator end() const noexcept { return const_iterator(m_order.end()); }

        /**
         * @brief Returns an iterator to the element following the last element of the unordered_set.
         * This element acts as a placeholder; attempting to access it results in undefined behavior.
         * @return Iterator to the element following the last element.
         */
        inline const_iterator cend() const noexcept { return const_iterator(m_order.cend()); }

        /**
         * @brief Returns a reverse iterator to the first element of the reversed ordered_set. It corresponds to the last element of the non-reversed ordered_set.
         * If the ordered_set is empty, the returned iterator is equal to rend().
         * @return Reverse iterator to the first element.
         */
        inline reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

        /**
         * @brief Returns a reverse iterator to the first element of the reversed ordered_set. It corresponds to the last element of the non-reversed ordered_set.
         * If the ordered_set is empty, the returned iterator is equal to rend().
         * @return Reverse iterator to the first element.
         */
        inline const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }

        /**
         * @brief Returns a reverse iterator to the first element of the reversed ordered_set. It corresponds to the last element of the non-reversed ordered_set.
         * If the ordered_set is empty, the returned iterator is equal to rend().
         * @return Reverse iterator to the first element.
         */
        inline const_reverse_iterator rcbegin() const noexcept { return const_reverse_iterator(cend()); }

        /**
         * @brief Returns a reverse iterator to the element following the last element of the reversed ordered_set.
         * It corresponds to the element preceding the first element of the non-reversed ordered_set. This element acts as a placeholder,
         * attempting to access it results in undefined behavior.
         * @return Reverse iterator to the element following the last element.
         */
        inline reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

        /**
         * @brief Returns a reverse iterator to the element following the last element of the reversed ordered_set.
         * It corresponds to the element preceding the first element of the non-reversed ordered_set. This element acts as a placeholder,
         * attempting to access it results in undefined behavior.
         * @return Reverse iterator to the element following the last element.
         */
        inline const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

        /**
         * @brief Returns a reverse iterator to the element following the last element of the reversed ordered_set.
         * It corresponds to the element preceding the first element of the non-reversed ordered_set. This element acts as a placeholder,
         * attempting to access it results in undefined behavior.
         * @return Reverse iterator to the element following the last element.
         */
        inline const_reverse_iterator rcend() const noexcept { return const_reverse_iterator(cbegin()); }
    private:
        std::list<value_type const*> m_order;
        unordered_set_type m_set;
        std::unordered_map<value_type const*, typename std::list<value_type const*>::const_iterator> m_lookup;

        friend class ordered_set_iterator<V, Hash, Pred, Alloc>;
    };
}
#endif