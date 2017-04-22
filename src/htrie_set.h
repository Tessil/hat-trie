/**
 * MIT License
 * 
 * Copyright (c) 2017 Tessil
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef TSL_HTRIE_SET_H
#define TSL_HTRIE_SET_H

#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <string>
#include <utility>
#include "htrie_hash.h"

namespace tsl {
    
/**
 * Implementation of a hat-trie set.
 * 
 * The size of a key string is limited to std::numeric_limits<KeySizeT>::max() - 1. 
 * That is 65 535 characters by default, but can be raised with the KeySizeT template parameter. 
 * See max_key_size() for an easy access to this limit.
 * 
 * Iterators invalidation:
 *  - clear, operator=: always invalidate the iterators.
 *  - insert: always invalidate the iterators.
 *  - erase: always invalidate the iterators.
 */ 
template<class CharT,
         class Hash = tsl::str_hash<CharT>,
         class KeySizeT = std::uint16_t>
class htrie_set {
private:
    using ht = tsl::detail_htrie_hash::htrie_hash<CharT, void, Hash, KeySizeT>;
    
public:
    using char_type = typename ht::char_type;
    using key_size_type = typename ht::key_size_type;
    using size_type = typename ht::size_type;
    using hasher = typename ht::hasher;
    using iterator = typename ht::iterator;
    using const_iterator = typename ht::const_iterator;
    
public:
    explicit htrie_set(const Hash& hash = Hash()): m_ht(hash, 
                                                        ht::HASH_NODE_DEFAULT_MAX_LOAD_FACTOR, 
                                                        ht::DEFAULT_BURST_THRESHOLD) 
    {
    }
    
    
    template<class InputIt>
    htrie_set(InputIt first, InputIt last,
             const Hash& hash = Hash()): htrie_set(hash)
    {
        insert(first, last);
    }
    
    
    
#ifdef TSL_HAS_STRING_VIEW
    htrie_set(std::initializer_list<std::basic_string_view<CharT>> init,
              const Hash& hash = Hash()): htrie_set(hash)
    {
        insert(init);
    }
#else    
    htrie_set(std::initializer_list<const CharT*> init,
              const Hash& hash = Hash()): htrie_set(hash)
    {
        insert(init);
    }
#endif
    
    

#ifdef TSL_HAS_STRING_VIEW
    htrie_set& operator=(std::initializer_list<std::basic_string_view<CharT>> ilist) {
        clear();
        insert(ilist);
        
        return *this;
    }
#else
    htrie_set& operator=(std::initializer_list<const CharT*> ilist) {
        clear();
        insert(ilist);
        
        return *this;
    }
#endif 
    
    
    
    /*
     * Iterators
     */
    iterator begin() noexcept { return m_ht.begin(); }
    const_iterator begin() const noexcept { return m_ht.begin(); }
    const_iterator cbegin() const noexcept { return m_ht.cbegin(); }
    
    iterator end() noexcept { return m_ht.end(); }
    const_iterator end() const noexcept { return m_ht.end(); }
    const_iterator cend() const noexcept { return m_ht.cend(); }
    
    
    /*
     * Capacity
     */
    bool empty() const noexcept { return m_ht.empty(); }
    size_type size() const noexcept { return m_ht.size(); }
    size_type max_size() const noexcept { return m_ht.max_size(); }
    size_type max_key_size() const noexcept { return m_ht.max_key_size(); }
    
        
    /*
     * Modifiers
     */
    void clear() noexcept { m_ht.clear(); }
    
    
    
#ifdef TSL_HAS_STRING_VIEW
    std::pair<iterator, bool> insert(const std::basic_string_view<CharT>& key) {
        return m_ht.insert(key.data(), key.size()); 
    }
#else
    std::pair<iterator, bool> insert(const CharT* key) {
        return m_ht.insert(key, std::strlen(key));
    }
    
    std::pair<iterator, bool> insert(const std::basic_string<CharT>& key) {
        return m_ht.insert(key.data(), key.size()); 
    }
#endif
    std::pair<iterator, bool> insert_ks(const CharT* key, size_type key_size) {
        return m_ht.insert(key, key_size);
    }
       
       
       
    template<class InputIt>
    void insert(InputIt first, InputIt last) {
        for(auto it = first; it != last; ++it) {
            insert(*it);
        }
    }
    
    

#ifdef TSL_HAS_STRING_VIEW
    void insert(std::initializer_list<std::basic_string_view<CharT>> ilist) {
        insert(ilist.begin(), ilist.end());
    }
#else
    void insert(std::initializer_list<const CharT*> ilist) {
        insert(ilist.begin(), ilist.end());
    }
#endif
    
    
    
    iterator erase(const_iterator pos) { return m_ht.erase(pos); }
    iterator erase(const_iterator first, const_iterator last) { return m_ht.erase(first, last); }

#ifdef TSL_HAS_STRING_VIEW
    size_type erase(const std::basic_string_view<CharT>& key) {
        return m_ht.erase(key.data(), key.size());
    }
#else
    size_type erase(const CharT* key) {
        return m_ht.erase(key, std::strlen(key));
    }
    
    size_type erase(const std::basic_string<CharT>& key) {
        return m_ht.erase(key.data(), key.size());
    }
#endif
    size_type erase_ks(const CharT* key, size_type key_size) {
        return m_ht.erase(key, key_size);
    }
    
    
    
    void swap(htrie_set& other) { other.m_ht.swap(m_ht); }
    
    
    /*
     * Lookup
     */
#ifdef TSL_HAS_STRING_VIEW 
    size_type count(const std::basic_string_view<CharT>& key) const { return m_ht.count(key.data(), key.size()); }
#else
    size_type count(const CharT* key) const { return m_ht.count(key, std::strlen(key)); }
    size_type count(const std::basic_string<CharT>& key) const { return m_ht.count(key.data(), key.size()); }
#endif
    size_type count_ks(const CharT* key, size_type key_size) const { return m_ht.count(key, key_size); }
    
    

#ifdef TSL_HAS_STRING_VIEW 
    iterator find(const std::basic_string_view<CharT>& key) {
        return m_ht.find(key.data(), key.size());
    }
    
    const_iterator find(const std::basic_string_view<CharT>& key) const {
        return m_ht.find(key.data(), key.size());
    }
#else
    iterator find(const CharT* key) {
        return m_ht.find(key, std::strlen(key));
    }
    
    const_iterator find(const CharT* key) const {
        return m_ht.find(key, std::strlen(key));
    }
    
    iterator find(const std::basic_string<CharT>& key) {
        return m_ht.find(key.data(), key.size());
    }
    
    const_iterator find(const std::basic_string<CharT>& key) const {
        return m_ht.find(key.data(), key.size());
    }
#endif    
    iterator find_ks(const CharT* key, size_type key_size) {
        return m_ht.find(key, key_size);
    }
    
    const_iterator find_ks(const CharT* key, size_type key_size) const {
        return m_ht.find(key, key_size);
    }
    

    
#ifdef TSL_HAS_STRING_VIEW 
    std::pair<iterator, iterator> equal_range(const std::basic_string_view<CharT>& key) {
        return m_ht.equal_range(key.data(), key.size());
    }
    
    std::pair<const_iterator, const_iterator> equal_range(const std::basic_string_view<CharT>& key) const {
        return m_ht.equal_range(key.data(), key.size());
    }
#else
    std::pair<iterator, iterator> equal_range(const CharT* key) {
        return m_ht.equal_range(key, std::strlen(key));
    }
    
    std::pair<const_iterator, const_iterator> equal_range(const CharT* key) const {
        return m_ht.equal_range(key, std::strlen(key));
    }
    
    std::pair<iterator, iterator> equal_range(const std::basic_string<CharT>& key) {
        return m_ht.equal_range(key.data(), key.size());
    }
    
    std::pair<const_iterator, const_iterator> equal_range(const std::basic_string<CharT>& key) const {
        return m_ht.equal_range(key.data(), key.size());
    }
#endif    
    std::pair<iterator, iterator> equal_range_ks(const CharT* key, size_type key_size) {
        return m_ht.equal_range(key, key_size);
    }
    
    std::pair<const_iterator, const_iterator> equal_range_ks(const CharT* key, size_type key_size) const {
        return m_ht.equal_range(key, key_size);
    }
    

    
#ifdef TSL_HAS_STRING_VIEW 
    std::pair<iterator, iterator> equal_prefix_range(const std::basic_string_view<CharT>& key) {
        return m_ht.equal_prefix_range(key.data(), key.size());
    }
    
    std::pair<const_iterator, const_iterator> equal_prefix_range(const std::basic_string_view<CharT>& key) const {
        return m_ht.equal_prefix_range(key.data(), key.size());
    }
#else
    std::pair<iterator, iterator> equal_prefix_range(const CharT* key) {
        return m_ht.equal_prefix_range(key, std::strlen(key));
    }
    
    std::pair<const_iterator, const_iterator> equal_prefix_range(const CharT* key) const {
        return m_ht.equal_prefix_range(key, std::strlen(key));
    }
    
    std::pair<iterator, iterator> equal_prefix_range(const std::basic_string<CharT>& key) {
        return m_ht.equal_prefix_range(key.data(), key.size());
    }
    
    std::pair<const_iterator, const_iterator> equal_prefix_range(const std::basic_string<CharT>& key) const {
        return m_ht.equal_prefix_range(key.data(), key.size());
    }
#endif    
    std::pair<iterator, iterator> equal_prefix_range_ks(const CharT* key, size_type key_size) {
        return m_ht.equal_prefix_range(key, key_size);
    }
    
    std::pair<const_iterator, const_iterator> equal_prefix_range_ks(const CharT* key, size_type key_size) const {
        return m_ht.equal_prefix_range(key, key_size);
    }
    
    
    /*
     *  Hash policy 
     */
    float max_load_factor() const { return m_ht.max_load_factor(); }
    void max_load_factor(float ml) { m_ht.max_load_factor(ml); }
    
    
    /*
     * Burst policy
     */
    size_type burst_threshold() const { return m_ht.burst_threshold(); }
    void burst_threshold(size_type threshold) { m_ht.burst_threshold(threshold); }
    
    
    /*
     * Observers
     */
    hasher hash_function() const { return m_ht.hash_function(); }
    
    
    
    /*
     * Other
     */
    friend bool operator==(const htrie_set& lhs, const htrie_set& rhs) {
        if(lhs.size() != rhs.size()) {
            return false;
        }
        
        std::string key_buffer;
        for(auto it = lhs.cbegin(); it != lhs.cend(); ++it) {
            it.key(key_buffer);
            
            const auto it_element_rhs = rhs.find(key_buffer);
            if(it_element_rhs == rhs.cend()) {
                return false;
            }
        }
        
        return true;
    }

    friend bool operator!=(const htrie_set& lhs, const htrie_set& rhs) {
        return !operator==(lhs, rhs);
    }

    friend void swap(htrie_set& lhs, htrie_set& rhs) {
        lhs.swap(rhs);
    }
    
private:
    ht m_ht;
};

} // end namespace tsl

#endif
