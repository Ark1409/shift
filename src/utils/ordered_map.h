#ifndef SHIFT_ORDERED_MAP_H_
#define SHIFT_ORDERED_MAP_H_ 1

#include "ordered_map_iterator.h"
#include "reverse_iterator.h"

#include <initializer_list>
#include <algorithm>

namespace shift::utils {
    /**
     * @brief ordered_map is an ordered container that contains key-value pairs with unique keys. Search, insertion,
     * and removal of elements have average constant-time complexity. Internally, this uses std::unordered_map and std::list
     * in order to provide (average) constant-time complexity.
     * 
     * @tparam K Type of key objects.
     * @tparam V Type of mapped objects.
     * @tparam Hash Hashing function object type, defaults to std::hash<K>.
     * @tparam Pred Predicate function object type, defaults to std::equal_to<K>.
     * @tparam Alloc Allocator type, defaults to std::allocator<std::pair<const K, V>>.
     */
    template<typename K, typename V, typename Hash = std::hash<K>, typename Pred = std::equal_to<K>,
        typename Alloc = std::allocator<std::pair<const K, V>>>
    class ordered_map {
    public:
        typedef typename std::unordered_map<K, V, Hash, Pred, Alloc>::key_type key_type;
        typedef typename std::unordered_map<K, V, Hash, Pred, Alloc>::mapped_type mapped_type;
        typedef typename std::unordered_map<K, V, Hash, Pred, Alloc>::value_type value_type;
        typedef typename std::unordered_map<K, V, Hash, Pred, Alloc>::size_type size_type;
        typedef typename std::unordered_map<K, V, Hash, Pred, Alloc>::difference_type difference_type;
        typedef typename std::unordered_map<K, V, Hash, Pred, Alloc>::hasher hasher;
        typedef typename std::unordered_map<K, V, Hash, Pred, Alloc>::key_equal key_equal;
        typedef typename std::unordered_map<K, V, Hash, Pred, Alloc>::reference reference;
        typedef typename std::unordered_map<K, V, Hash, Pred, Alloc>::const_reference const_reference;
        typedef typename std::unordered_map<K, V, Hash, Pred, Alloc>::pointer pointer;
        typedef typename std::unordered_map<K, V, Hash, Pred, Alloc>::const_pointer const_pointer;

        typedef ordered_map_iterator<K, V, Hash, Pred, Alloc> iterator;
        typedef ordered_map_iterator<K, V const, Hash, Pred, Alloc> const_iterator;

        typedef utils::reverse_iterator<iterator> reverse_iterator;
        typedef utils::reverse_iterator<const_iterator> const_reverse_iterator;
    private:
        typedef ordered_map<K, V, Hash, Pred, Alloc> ordered_map_type;
    public:
        /**
         * @brief Default constructor. Constructs an empty container with a default-constructed allocator.
         */
        ordered_map() = default;

        /**
         * @brief Copy constructor. Constructs the container with the copy of the contents of other.
         * @param other another container to be used as source to initialize the elements of the container with
         */
        ordered_map(const ordered_map& other) = default;

        /**
         * @brief Move constructor. Constructs the container with the contents of other using move semantics.
         * @param other another container to be used as source to initialize the elements of the container with
         */
        ordered_map(ordered_map&& other) = default;

        /**
         * @brief Constructs the container with the contents of the range [first, last). Sets max_load_factor() to 1.0.
         * If multiple elements in the range have keys that compare equivalent, the last one is kept.
         * @tparam InputIt must meet the requirements of LegacyInputIterator.
         * @param first the range to copy the elements from
         * @param last the range to copy the elements from
         */
        template< class InputIt >
        inline ordered_map(InputIt first, InputIt last) {
            for (; first != last; ++first) {
                push_back(*first);
            }
        }

        /**
         * @brief Constructs the container with the contents of the initializer list init, same as ordered_map(init.begin(), init.end()).
         * If multiple elements in the range have keys that compare equivalent, the last one is kept.
         * @param init initializer list to initialize the elements of the container with
         */
        inline ordered_map(std::initializer_list<value_type> init) : ordered_map(init.begin(), init.end()) { }

        /**
         * @brief Copy assignment operator. Replaces the contents with a copy of the contents of other.
         * @param other another container to use as data source
         * @return @c *this
         */
        ordered_map& operator=(const ordered_map& other) = default;

        /**
         * @brief Move assignment operator. Replaces the contents with those of other using move semantics (i.e. the data in other is moved from other into this container).
         * other is in a valid but unspecified state afterwards.
         * @param other another container to use as data source
         * @return *this
         */
        ordered_map& operator=(ordered_map&& other) = default;

        /**
         * @brief Checks if the container has no elements, i.e. whether begin() == end().
         * @return true if the container is empty, false otherwise
         */
        inline bool empty() const noexcept { return m_map.empty(); }

        /**
         * @brief Returns the number of elements in the container, i.e. std::distance(begin(), end()).
         * @return The number of elements in the container.
         */
        inline size_type size() const noexcept { return m_map.size(); }

        /**
         * @brief Returns the maximum number of elements the container is able to hold due to system or library implementation limitations, i.e. std::distance(begin(), end()) for the largest container.
         * @return Maximum number of elements.
         */
        inline size_type max_size() const noexcept { return std::min(m_order.max_size(), std::min(m_map.max_size(), m_lookup_map.max_size())); }

        /**
         * @brief Erases all elements from the container. After this call, size() returns zero.
         * Invalidates any references, pointers, or iterators referring to contained elements. May also invalidate past-the-end iterators.
         */
        inline void clear() noexcept { m_order.clear(); m_map.clear(); m_lookup_map.clear(); }

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
        inline void pop_front() { if (size() > 0) { erase(m_lookup_map[m_order.front()]); } }

        /**
         * @brief Removes the last element of the container. Calling pop_back on an empty container is a no-op.
         * References and iterators to the erased element are invalidated.
         */
        inline void pop_back() { if (size() > 0) { erase(m_lookup_map[m_order.back()]); } }

        /**
         * @brief Inserts element(s) into the container. If the container doesn't already contain an element with an equivalent key,
         * the element is inserted at the desired position. If the element already exists within the container, it is relocated and has its value reassigned.
         * @param pos iterator before which the content will be inserted (pos may be the end() iterator)
         * @param value element value to insert
         * @return Returns an iterator to the inserted element
         */
        iterator insert(const_iterator pos, const value_type& value) {
            auto c = m_map.find(value.first);
            if (c == m_map.end()) {
                auto r = m_map.insert(value);
                auto i = m_order.insert(pos.m_order_it, r.first.operator->());
                m_lookup_map[r.first.operator->()] = i;
                return iterator(i);
            } else {
                auto f = m_lookup_map.find(c.operator->());
                typename std::list<value_type*>::const_iterator p_it = f->second;
                (*p_it)->second = value.second;
                auto i = m_order.insert(pos.m_order_it, (*p_it));
                f->second = i;
                m_order.erase(p_it);
                return iterator(i);
            }
        }

        /**
         * @brief Inserts element(s) into the container. If the container doesn't already contain an element with an equivalent key,
         * the element is inserted at the desired position. If the element already exists within the container, its is relocated and has its value reassigned.
         * @param pos iterator before which the content will be inserted (pos may be the end() iterator)
         * @param value element value to insert
         * @return Returns an iterator to the inserted element
         */
        iterator insert(const_iterator pos, value_type&& value) {
            auto c = m_map.find(value.first);
            if (c == m_map.end()) {
                auto r = m_map.insert(std::move(value));
                auto i = m_order.insert(pos.m_order_it, r.first.operator->());
                m_lookup_map[r.first.operator->()] = i;
                return iterator(i);
            } else {
                auto f = m_lookup_map.find(c.operator->());
                typename std::list<value_type*>::const_iterator p_it = f->second;
                (*p_it)->second = std::move(value.second);
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
            m_map.erase(pos->first);
            m_lookup_map.erase(*pos.m_order_it);
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
            auto f = m_map.find(key);
            if (f == m_map.end()) return 0;
            erase(const_iterator(m_lookup_map[f.operator->()]));
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
        inline void swap(ordered_map& other) noexcept(noexcept(std::unordered_map<K, V, Hash, Pred, Alloc>::swap)) { m_order.swap(other.m_order); m_map.swap(other.m_map); m_lookup_map.swap(other.m_lookup_map); }

        /**
         * @brief Sorts the elements in ascending order. The order of equal elements is preserved. The first version uses operator< to compare the elements,
         * the second version uses the given comparison function comp. If an exception is thrown, the order of elements in *this is unspecified.
         */
        inline void sort() { return sort([](value_type const& a, value_type const& b) { return std::less<value_type>()(a, b); }); }

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
            auto look = m_map.find(key);
            if (look == m_map.end()) { return end(); }
            auto f = m_lookup_map.find(look.operator->());
            return iterator(f->second);
        }

        /**
         * @brief Finds an element with key equivalent to key. This function has constant complexity on average, worst case linear in the size of the container.
         * @param key key value of the element to search for
         * @return Iterator to an element with key equivalent to key. If no such element is found, past-the-end (see end()) iterator is returned.
         */
        inline const_iterator find(const key_type& key) const {
            auto look = m_map.find(key);
            if (look == m_map.end()) { return end(); }
            auto f = m_lookup_map.find(look.operator->());
            return const_iterator(f->second);
        }

        /**
         * @brief Returns a reference to the mapped value of the element with key equivalent to key. If no such element exists, an exception of type std::out_of_range is thrown.
         * @param key the key of the element to find
         * @return Reference to the mapped value of the requested element.
         */
        inline mapped_type& at(const key_type& key) {
            auto f = m_map.find(key);
            if (f != m_map.end()) return f->second;
            throw std::out_of_range("ordered_map::at");
        }

        /**
         * @brief Returns a reference to the mapped value of the element with key equivalent to key. If no such element exists, an exception of type std::out_of_range is thrown.
         * @param key the key of the element to find
         * @return Reference to the mapped value of the requested element.
         */
        inline mapped_type const& at(const key_type& key) const {
            auto f = m_map.find(key);
            if (f != m_map.cend()) return f->second;
            throw std::out_of_range("ordered_map::at");
        }

        /**
         * @brief Returns a reference to the value that is mapped to a key equivalent to key, performing an insertion if such key does not already exist.
         * @param key the key of the element to find
         * @return Reference to the mapped value of the new element if no element with key key existed. Otherwise a reference to the mapped value of the existing element whose key is equivalent to key.
         */
        inline mapped_type& operator[](const key_type& key) {
            auto f = m_map.find(key);
            if (f != m_map.end()) return f->second;
            push_back(value_type(key, mapped_type()));
            return back().second;
        }

        /**
         * @brief Returns a reference to the value that is mapped to a key equivalent to key, performing an insertion if such key does not already exist.
         * @param key the key of the element to find
         * @return Reference to the mapped value of the new element if no element with key key existed. Otherwise a reference to the mapped value of the existing element whose key is equivalent to key.
         */
        inline mapped_type& operator[](key_type&& key) {
            auto f = m_map.find(key);
            if (f != m_map.end()) return f->second;
            push_back(value_type(std::move(key), mapped_type()));
            return back().second;
        }

        /**
         * @brief Returns a reference to the value that is mapped to a key equivalent to key. If no such element exists, an exception of type std::out_of_range is thrown.
         * @param key the key of the element to find
         * @return A reference to the mapped value of the existing element whose key is equivalent to key.
         */
        inline mapped_type const& operator[](const key_type& key) const { return at(key); }

        /**
         * @brief Checks if there is an element with key equivalent to key in the container.
         * @param key key value of the element to search for
         * @return true if there is such an element, otherwise false.
         */
        inline bool contains(const key_type& key) const { return m_map.find(key) != m_map.end(); }

        /**
         * @brief Returns a reference to the last element in the container. Calling back on an empty container causes undefined behavior.
         * @return Reference to the last element.
         */
        inline value_type& back() noexcept { return *m_order.back(); }

        /**
         * @brief Returns a reference to the last element in the container. Calling back on an empty container causes undefined behavior.
         * @return Reference to the last element.
         */
        inline value_type const& back() const noexcept { return *m_order.back(); }

        /**
         * @brief Returns a reference to the first element in the container. Calling front on an empty container causes undefined behavior.
         * @return Reference to the first element
         */
        inline value_type& front() noexcept { return *m_order.front(); }

        /**
         * @brief Returns a reference to the first element in the container. Calling front on an empty container causes undefined behavior.
         * @return Reference to the first element
         */
        inline value_type const& front() const noexcept { return *m_order.front(); }

        /**
         * @brief Returns an iterator to the first element of the unordered_map. If the unordered_map is empty, the returned iterator will be equal to end().
         * @return Iterator to the first element.
         */
        inline iterator begin() noexcept { return iterator(m_order.begin()); }

        /**
         * @brief Returns an iterator to the first element of the unordered_map. If the unordered_map is empty, the returned iterator will be equal to end().
         * @return Iterator to the first element.
         */
        inline const_iterator begin() const noexcept { return const_iterator(m_order.begin()); }

        /**
         * @brief Returns an iterator to the first element of the unordered_map. If the unordered_map is empty, the returned iterator will be equal to end().
         * @return Iterator to the first element.
         */
        inline const_iterator cbegin() const noexcept { return const_iterator(m_order.cbegin()); }

        /**
         * @brief Returns an iterator to the element following the last element of the unordered_map.
         * This element acts as a placeholder; attempting to access it results in undefined behavior.
         * @return Iterator to the element following the last element.
         */
        inline iterator end() noexcept { return iterator(m_order.end()); }

        /**
         * @brief Returns an iterator to the element following the last element of the unordered_map.
         * This element acts as a placeholder; attempting to access it results in undefined behavior.
         * @return Iterator to the element following the last element.
         */
        inline const_iterator end() const noexcept { return const_iterator(m_order.end()); }

        /**
         * @brief Returns an iterator to the element following the last element of the unordered_map.
         * This element acts as a placeholder; attempting to access it results in undefined behavior.
         * @return Iterator to the element following the last element.
         */
        inline const_iterator cend() const noexcept { return const_iterator(m_order.cend()); }

        /**
         * @brief Returns a reverse iterator to the first element of the reversed ordered_map. It corresponds to the last element of the non-reversed ordered_map.
         * If the ordered_map is empty, the returned iterator is equal to rend().
         * @return Reverse iterator to the first element.
         */
        inline reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

        /**
         * @brief Returns a reverse iterator to the first element of the reversed ordered_map. It corresponds to the last element of the non-reversed ordered_map.
         * If the ordered_map is empty, the returned iterator is equal to rend().
         * @return Reverse iterator to the first element.
         */
        inline const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }

        /**
         * @brief Returns a reverse iterator to the first element of the reversed ordered_map. It corresponds to the last element of the non-reversed ordered_map.
         * If the ordered_map is empty, the returned iterator is equal to rend().
         * @return Reverse iterator to the first element.
         */
        inline const_reverse_iterator rcbegin() const noexcept { return const_reverse_iterator(cend()); }

        /**
         * @brief Returns a reverse iterator to the element following the last element of the reversed ordered_map.
         * It corresponds to the element preceding the first element of the non-reversed ordered_map. This element acts as a placeholder,
         * attempting to access it results in undefined behavior.
         * @return Reverse iterator to the element following the last element.
         */
        inline reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

        /**
         * @brief Returns a reverse iterator to the element following the last element of the reversed ordered_map.
         * It corresponds to the element preceding the first element of the non-reversed ordered_map. This element acts as a placeholder,
         * attempting to access it results in undefined behavior.
         * @return Reverse iterator to the element following the last element.
         */
        inline const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

        /**
         * @brief Returns a reverse iterator to the element following the last element of the reversed ordered_map.
         * It corresponds to the element preceding the first element of the non-reversed ordered_map. This element acts as a placeholder,
         * attempting to access it results in undefined behavior.
         * @return Reverse iterator to the element following the last element.
         */
        inline const_reverse_iterator rcend() const noexcept { return const_reverse_iterator(cbegin()); }
    private:
        std::list<value_type*> m_order;
        std::unordered_map<K, V, Hash, Pred, Alloc> m_map;
        std::unordered_map<value_type const*, typename std::list<value_type*>::const_iterator> m_lookup_map;

        friend struct ordered_map_iterator<K, V, Hash, Pred, Alloc>;
    };
}
#endif