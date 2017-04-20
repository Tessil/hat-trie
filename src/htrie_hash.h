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
#ifndef TSL_HTRIE_HASH_H
#define TSL_HTRIE_HASH_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "array-hash/src/array_map.h"
#include "array-hash/src/array_set.h"

namespace tsl {       
    
namespace detail_htrie_hash {
    
template<class T>
struct value_node {
    template<class... Args>
    value_node(Args&&... args): m_value(std::forward<Args>(args)...) {
    }
    
    T m_value;
};

template<>
struct value_node<void> {
};
    
    

template<class CharT,
         class T, 
         class Hash,
         class KeySizeT>
class htrie_hash {
    template<typename U>
    using has_value = typename std::integral_constant<bool, !std::is_same<U, void>::value>;
    
    static_assert(std::is_same<CharT, char>::value, "char is the only supported CharT type for now.");
    
    static const std::size_t ALPHABET_SIZE = std::numeric_limits<unsigned char>::max() + 1;
    
public:
    template<bool is_const>
    class htrie_hash_iterator_with_value;
    
    template<bool is_const>
    class htrie_hash_iterator_without_value;
    
    
    
    using char_type = CharT;
    using key_size_type = KeySizeT;
    using size_type = std::size_t;
    using hasher = Hash;
    using iterator = typename std::conditional<has_value<T>::value, 
                                               htrie_hash_iterator_with_value<false>, 
                                               htrie_hash_iterator_without_value<false>>::type;
    using const_iterator = typename std::conditional<has_value<T>::value, 
                                                     htrie_hash_iterator_with_value<true>, 
                                                     htrie_hash_iterator_without_value<true>>::type;
    
    
private:
    using array_hash = typename std::conditional<
                                has_value<T>::value, 
                                tsl::array_map<CharT, T, Hash, std::char_traits<CharT>, false, 
                                            KeySizeT, std::uint16_t, tsl::power_of_two_growth_policy<4>>,
                                tsl::array_set<CharT, Hash, std::char_traits<CharT>, false, 
                                            KeySizeT, std::uint16_t, tsl::power_of_two_growth_policy<4>>>::type;
    
private:
    /**
     * The tree is mainly composed of two nodes: trie_node and hash_node which both have anode a base class.
     * A trie_node should have at least one child, but it can have more than that. Each child is either an
     * hash_node or a trie_node.
     * 
     * A hash_node is always a leaf node, it doesn't have any child.
     * 
     * Example:
     *      | ... | a |.. ..................... | f | ... | trie_node_1
     *               \                             \
     * hash_node_1 |array_hash = {"dd"}|     |...| a | ... | trie_node_2  
     *                                             /
     *                     |array_hash = {"ble", "bric", "lse"}| hash_node_2
     * 
     * 
     * Each trie_node may also have a value node if the trie_node marks the end of a string value.
     */
    
    using value_node = tsl::detail_htrie_hash::value_node<T>;
    
    struct trie_node;
    struct hash_node;
    
    struct anode {
        virtual ~anode() = default;
        
        bool is_trie_node() const noexcept {
            return m_node_type == node_type::TRIE_NODE;
        }
        
        bool is_hash_node() const noexcept {
            return m_node_type == node_type::HASH_NODE;
        }
        
        trie_node& as_trie_node() noexcept {
            tsl_assert(is_trie_node());
            return static_cast<trie_node&>(*this);
        }
        
        hash_node& as_hash_node() noexcept {
            tsl_assert(is_hash_node());
            return static_cast<hash_node&>(*this);
        }
        
        const trie_node& as_trie_node() const noexcept {
            tsl_assert(is_trie_node());
            return static_cast<const trie_node&>(*this);
        }
        
        const hash_node& as_hash_node() const noexcept {
            tsl_assert(is_hash_node());
            return static_cast<const hash_node&>(*this);
        }
        
    protected:
        enum class node_type: unsigned char {
            HASH_NODE,
            TRIE_NODE
        };
    
        anode(node_type node_type_): m_node_type(node_type_), m_child_of_char(0) {
        }
    
        anode(node_type node_type_, char child_of_char): m_node_type(node_type_), 
                                                                  m_child_of_char(child_of_char) 
        {
        }
        
        node_type m_node_type;
        
    public:
        /*
         * If the node has a parent, then it's a descendent of some char.
         * 
         * Example:
         *      | ... | a | b | ... | trie_node_1
         *                   \
         *              |...| a | ... | trie_node_2  
         *                   /
         *            |array_hash| hash_node_1
         *   
         * trie_node_2 is a child of trie_node_1 through 'b', it will have 'b' as m_child_of_char.
         * hash_node_1 is a child of trie_node_2 through 'a', it will have 'a' as m_child_of_char.
         * 
         * trie_node_1 has no parent, its m_child_of_char is undeterminated.
         */
        char m_child_of_char;
    };
    
    
    struct trie_node : public anode {
        trie_node(): anode(anode::node_type::TRIE_NODE), m_parent_node(nullptr),
                     m_value_node(nullptr), m_children() 
        {
        }
        
        trie_node(const trie_node& other): anode(anode::node_type::TRIE_NODE, other.m_child_of_char),
                                           m_value_node(nullptr), m_children()
        {
            if(other.m_value_node != nullptr) {
                // Cast to call the copy constructor instead of the variadic constructor.
                m_value_node.reset(new value_node(static_cast<const value_node&>(*other.m_value_node)));
            }
            
            // TODO avoid recursion
            for(std::size_t ichild = 0; ichild < other.m_children.size(); ichild++) {
                if(other.m_children[ichild] != nullptr) {
                    if(other.m_children[ichild]->is_hash_node()) {
                        m_children[ichild].reset(new hash_node(other.m_children[ichild]->as_hash_node()));
                    }
                    else {
                        m_children[ichild].reset(new trie_node(other.m_children[ichild]->as_trie_node()));
                    }
                }
            }
        }
        
        trie_node(trie_node&& other) = delete;
        trie_node& operator=(const trie_node& other) = delete;
        trie_node& operator=(trie_node&& other) = delete;
        
        const anode& first_child() const {
            for(const auto& node: m_children) {
                if(node != nullptr) {
                    return *node;
                }
            }
            
            throw std::runtime_error("No child in the trie node.");
        }
        
        anode& first_child() {
            return const_cast<anode&>(static_cast<const trie_node*>(this)->first_child()); 
        }
        
        
        bool empty() const noexcept {
            for(const auto& node: m_children) {
                if(node != nullptr) {
                    return false;
                }
            }
            
            return true;
        }
        
        
        trie_node* m_parent_node;
        std::unique_ptr<value_node> m_value_node;
        std::array<std::unique_ptr<anode>, ALPHABET_SIZE> m_children;
    };

    struct hash_node : public anode {
        hash_node(const Hash& hash, float max_load_factor): 
                hash_node(HASH_NODE_DEFAULT_INIT_BUCKETS_COUNT, hash, max_load_factor) 
        {
        }
        
        hash_node(size_type bucket_count, const Hash& hash, float max_load_factor): 
                anode(anode::node_type::HASH_NODE), m_array_hash(bucket_count, hash) 
        {
            m_array_hash.max_load_factor(max_load_factor);
        }
        
        hash_node(const hash_node& other): anode(anode::node_type::HASH_NODE, other.m_child_of_char), 
                                           m_array_hash(other.m_array_hash) 
        {
        }
        
        hash_node(hash_node&& other) = delete;
        hash_node& operator=(const hash_node& other) = delete;
        hash_node& operator=(hash_node&& other) = delete;
        
        
        array_hash m_array_hash;
    };
    
    
    
   
private: 
    /**
     * From 'from_node' go down through the first child node of each trie node
     * and return the first trie node with an m_value_node. If none return the most left trie node.
     */
    static const trie_node& most_left_trie_node(const trie_node& from_node) {
        const trie_node* current_node = &from_node;
        while(true) {
            if(current_node->m_value_node != nullptr) {
                return *current_node;
            }
            
            const anode& first_child = current_node->first_child();
            if(first_child.is_hash_node()) {
                return *current_node;
            }
            
            current_node = &first_child.as_trie_node();
        }
    }
    
    static trie_node& most_left_trie_node(trie_node& from) {
        return const_cast<trie_node&>(most_left_trie_node(static_cast<const trie_node&>(from)));
    }
    
    static std::size_t as_position(char c) noexcept {
        return static_cast<std::size_t>(static_cast<unsigned char>(c));
    }
    
    
    
private:
    template<bool is_const>
    class htrie_hash_iterator {
        friend class htrie_hash_iterator<true>;
        
    protected:
        using anode_type = typename std::conditional<is_const, const anode, anode>::type;
        using trie_node_type = typename std::conditional<is_const, const trie_node, trie_node>::type;
        using hash_node_type = typename std::conditional<is_const, const hash_node, hash_node>::type;
        
        using array_hash_iterator_type = typename std::conditional<is_const, typename array_hash::const_iterator, 
                                                                             typename array_hash::iterator>::type;
        
    protected:
        htrie_hash_iterator(trie_node_type* start_trie_node, hash_node_type* first_hash_node) noexcept: 
            m_read_trie_node_value(false), m_current_trie_node(start_trie_node), m_current_hash_node(first_hash_node)
        {
            if(m_current_trie_node != nullptr) {
                m_read_trie_node_value = (m_current_trie_node->m_value_node != nullptr);
            }
            
            if(m_current_hash_node != nullptr) {
                m_array_hash_iterator = m_current_hash_node->m_array_hash.begin();
                m_array_hash_end_iterator = m_current_hash_node->m_array_hash.end();
            }
        }
        
        /*
         * Start reading from the value in start_trie_node. start_trie_node->m_value_node should be non-null.
         */
        htrie_hash_iterator(trie_node_type* start_trie_node) noexcept:
            m_read_trie_node_value(true), m_current_trie_node(start_trie_node), m_current_hash_node(nullptr)
        {
            tsl_assert(m_current_trie_node != nullptr);
            tsl_assert(m_current_trie_node->m_value_node != nullptr);
        }
        
        htrie_hash_iterator(trie_node_type* start_trie_node, hash_node_type* hnode, 
                          array_hash_iterator_type begin, array_hash_iterator_type end, 
                          bool read_trie_node_value) noexcept:
            m_read_trie_node_value(read_trie_node_value), m_current_trie_node(start_trie_node), 
            m_current_hash_node(hnode), m_array_hash_iterator(begin), m_array_hash_end_iterator(end)
        {
        }
        
    public:
        htrie_hash_iterator() noexcept {
        }
        
        htrie_hash_iterator(const htrie_hash_iterator<false>& other) noexcept: 
            m_read_trie_node_value(other.m_read_trie_node_value), m_current_trie_node(other.m_current_trie_node),
            m_current_hash_node(other.m_current_hash_node), m_array_hash_iterator(other.m_array_hash_iterator),
            m_array_hash_end_iterator(other.m_array_hash_end_iterator)
        {
        }
        
        void key(std::string& key_buffer_out) const {
            key_buffer_out.clear();
            
            trie_node_type* tnode = m_current_trie_node;
            while(tnode != nullptr && tnode->m_parent_node != nullptr) {
                key_buffer_out.push_back(tnode->m_child_of_char);
                tnode = tnode->m_parent_node;
            }
            
            std::reverse(key_buffer_out.begin(), key_buffer_out.end());
            
            if(!m_read_trie_node_value) {
                tsl_assert(m_current_hash_node != nullptr);
                if(m_current_trie_node != nullptr) {
                    key_buffer_out.push_back(m_current_hash_node->m_child_of_char);
                }
                
                key_buffer_out.append(m_array_hash_iterator.key(), m_array_hash_iterator.key_size());
            }
        }
        
        std::string key() const {
            std::string key_buffer;
            key(key_buffer);
            
            return key_buffer;
        }
        
        htrie_hash_iterator& operator++() {
            if(m_read_trie_node_value) {
                m_read_trie_node_value = false;
                set_next_node(m_current_trie_node->first_child());
                
                return *this;
            }
            
            
            ++m_array_hash_iterator;
            if(m_array_hash_iterator != m_array_hash_end_iterator) {
            }
            // End of the road, set the iterator as an end node.
            else if(m_current_trie_node == nullptr) {
                m_current_hash_node = nullptr;
            }
            else {
                tsl_assert(m_current_hash_node != nullptr);
                
                anode_type* next_node = next_trie_child(as_position(m_current_hash_node->m_child_of_char));
                while(next_node == nullptr && m_current_trie_node->m_parent_node != nullptr) {
                    anode_type* current_child = m_current_trie_node;
                    m_current_trie_node = m_current_trie_node->m_parent_node;
                    next_node = next_trie_child(as_position(current_child->m_child_of_char));
                }
                
                // End of the road, set the iterator as an end node.
                if(next_node == nullptr) {
                    m_current_trie_node = nullptr;
                    m_current_hash_node = nullptr;
                }
                else {
                    set_next_node(*next_node);
                }
            }
            
            return *this;
        }
        
        htrie_hash_iterator operator++(int) {
            htrie_hash_iterator tmp(*this);
            ++*this;
            
            return tmp;
        }
        
        friend bool operator==(const htrie_hash_iterator& lhs, const htrie_hash_iterator& rhs) { 
            if(lhs.m_current_trie_node != rhs.m_current_trie_node || 
               lhs.m_current_hash_node != rhs.m_current_hash_node ||
               lhs.m_read_trie_node_value != rhs.m_read_trie_node_value)
            {
                return false;
            }
            
            if(lhs.m_current_hash_node != nullptr) {
                tsl_assert(rhs.m_current_hash_node != nullptr);
                
                return lhs.m_array_hash_iterator == rhs.m_array_hash_iterator &&
                       lhs.m_array_hash_end_iterator == rhs.m_array_hash_end_iterator;
            }
            else {
                return true;
            }
        }
        
        friend bool operator!=(const htrie_hash_iterator& lhs, const htrie_hash_iterator& rhs) { 
            return !(lhs == rhs); 
        }
        
    protected:
        /**
         * Get the next child in the current trie node.
         * Return null if none.
         */
        anode_type* next_trie_child(std::size_t icurrent_child) const {
            tsl_assert(m_current_trie_node != nullptr);
            tsl_assert(icurrent_child < m_current_trie_node->m_children.size());
            
            for(std::size_t i = icurrent_child + 1; i < m_current_trie_node->m_children.size(); i++) {
                if(m_current_trie_node->m_children[i] != nullptr) {
                    return m_current_trie_node->m_children[i].get();
                }
            }
            
            return nullptr;
        }
        
        void set_next_node(anode_type& next_node) {
            if(next_node.is_hash_node()) {
                set_current_hash_node(next_node.as_hash_node());
            }
            else {
                m_current_trie_node = &most_left_trie_node(next_node.as_trie_node());
                if(m_current_trie_node->m_value_node != nullptr) {
                    m_read_trie_node_value = true;
                }
                else {
                    set_current_hash_node(m_current_trie_node->first_child().as_hash_node());
                }
            }
        }
        
        void set_current_hash_node(hash_node_type& hnode) {
            m_current_hash_node = &hnode;
            m_array_hash_iterator = m_current_hash_node->m_array_hash.begin();
            m_array_hash_end_iterator = m_current_hash_node->m_array_hash.end();
        }
        
    protected:
        bool m_read_trie_node_value;
        
        trie_node_type* m_current_trie_node;
        hash_node_type* m_current_hash_node;
        
        array_hash_iterator_type m_array_hash_iterator;
        array_hash_iterator_type m_array_hash_end_iterator;
        
    }; 
    
public:    
    template<bool is_const>
    class htrie_hash_iterator_with_value: private htrie_hash_iterator<is_const> {
        friend class htrie_hash;
        using base = htrie_hash_iterator<is_const>;
        
        
        htrie_hash_iterator_with_value(typename base::trie_node_type* start_trie_node, 
                                       typename base::hash_node_type* first_hash_node) noexcept: 
                        base(start_trie_node, first_hash_node) 
        {
        }
     
        htrie_hash_iterator_with_value(typename base::trie_node_type* start_trie_node) noexcept: 
                        base(start_trie_node)
        {
        }

        htrie_hash_iterator_with_value(typename base::trie_node_type* start_trie_node, 
                                       typename base::hash_node_type* hnode, 
                                       typename base::array_hash_iterator_type begin, 
                                       typename base::array_hash_iterator_type end,
                                       bool read_trie_node_value = false) noexcept: 
                        base(start_trie_node, hnode, begin, end, read_trie_node_value)
        {
        }
        
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = typename std::conditional<is_const, const T, T>::type;
        using difference_type = std::ptrdiff_t;
        using reference = value_type&;
        using pointer = value_type*;
        
    public:
        htrie_hash_iterator_with_value() noexcept {
        }
        
        htrie_hash_iterator_with_value(const htrie_hash_iterator_with_value<false>& other) noexcept : base(other) {
        }
        
        void key(std::string& key_buffer_out) const {
            return base::key(key_buffer_out);
        }
        
        std::string key() const {
            return base::key();
        }
        
        reference value() const {
            if(this->m_read_trie_node_value) {
                tsl_assert(this->m_current_trie_node != nullptr && this->m_current_trie_node->m_value_node != nullptr);
                return this->m_current_trie_node->m_value_node->m_value;
            }
            else {
                return this->m_array_hash_iterator.value();
            }
        }
        
        reference operator*() const {
            return value();
        }
        
        pointer operator->() const {
            return &value();
        }
        
        htrie_hash_iterator_with_value& operator++() {
            base::operator++(); 
            return *this;
        }
        
        htrie_hash_iterator_with_value operator++(int) {
            return base::operator++(0);
        }
        
        friend bool operator==(const htrie_hash_iterator_with_value& lhs, const htrie_hash_iterator_with_value& rhs) { 
            return static_cast<htrie_hash_iterator<is_const>>(lhs) == static_cast<htrie_hash_iterator<is_const>>(rhs); 
        }
        
        friend bool operator!=(const htrie_hash_iterator_with_value& lhs, const htrie_hash_iterator_with_value& rhs) { 
            return !(lhs == rhs); 
        }
    };
    
    template<bool is_const>
    class htrie_hash_iterator_without_value: private htrie_hash_iterator<is_const> {
        friend class htrie_hash;
        using base = htrie_hash_iterator<is_const>;
        
        htrie_hash_iterator_without_value(typename base::trie_node_type* start_trie_node, 
                                          typename base::hash_node_type* first_hash_node) noexcept: 
                        base(start_trie_node, first_hash_node) 
        {
        }
     
        htrie_hash_iterator_without_value(typename base::trie_node_type* start_trie_node) noexcept: 
                        base(start_trie_node)
        {
        }

        htrie_hash_iterator_without_value(typename base::trie_node_type* start_trie_node, 
                                          typename base::hash_node_type* hnode, 
                                          typename base::array_hash_iterator_type begin, 
                                          typename base::array_hash_iterator_type end,
                                          bool read_trie_node_value = false) noexcept: 
                        base(start_trie_node, hnode, begin, end, read_trie_node_value)
        {
        }
        
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = void;
        using difference_type = std::ptrdiff_t;
        using reference = void;
        using pointer = void;
        
    public:
        htrie_hash_iterator_without_value() noexcept {
        }
        
        htrie_hash_iterator_without_value(const htrie_hash_iterator_without_value<false>& other) noexcept: base(other) {
        }
        
        void key(std::string& key_buffer_out) const {
            return base::key(key_buffer_out);
        }
        
        std::string key() const {
            return base::key();
        }
        
        htrie_hash_iterator_without_value& operator++() {
            base::operator++(); 
            return *this;
        }
        
        htrie_hash_iterator_without_value operator++(int) {
            return base::operator++(0);
        }
        
        friend bool operator==(const htrie_hash_iterator_without_value& lhs, const htrie_hash_iterator_without_value& rhs) { 
            return static_cast<htrie_hash_iterator<is_const>>(lhs) == static_cast<htrie_hash_iterator<is_const>>(rhs); 
        }
        
        friend bool operator!=(const htrie_hash_iterator_without_value& lhs, const htrie_hash_iterator_without_value& rhs) { 
            return !(lhs == rhs); 
        }
    };


    
    
    
public:
    htrie_hash(const Hash& hash, float max_load_factor): m_root(nullptr), m_nb_elements(0), 
                                  m_hash(hash), m_max_load_factor(max_load_factor) 
    {
    }
    
    htrie_hash(const htrie_hash& other): m_root(nullptr), m_nb_elements(other.m_nb_elements), 
                                         m_hash(other.m_hash), m_max_load_factor(other.m_max_load_factor) 
    {
        if(other.m_root != nullptr) {
            if(other.m_root->is_hash_node()) {
                m_root.reset(new hash_node(other.m_root->as_hash_node()));
            }
            else {
                m_root.reset(new trie_node(other.m_root->as_trie_node()));
            }
        }
    }
    
    htrie_hash(htrie_hash&& other) noexcept(std::is_nothrow_move_constructible<Hash>::value)
                                  : m_root(std::move(other.m_root)),
                                    m_nb_elements(other.m_nb_elements),
                                    m_hash(std::move(other.m_hash)),
                                    m_max_load_factor(other.m_max_load_factor)
    {
        other.clear();
    }
    
    htrie_hash& operator=(const htrie_hash& other) {
        if(&other != this) {
            std::unique_ptr<anode> new_root = nullptr;
            if(other.m_root != nullptr) {
                if(other.m_root->is_hash_node()) {
                    new_root.reset(new hash_node(other.m_root->as_hash_node()));
                }
                else {
                    new_root.reset(new trie_node(other.m_root->as_trie_node()));
                }
            }
            
            m_hash = other.m_hash;
            m_root = std::move(new_root);
            m_nb_elements = other.m_nb_elements;
            m_max_load_factor = other.m_max_load_factor;
        }
        
        return *this;
    }
    
    htrie_hash& operator=(htrie_hash&& other) {
        other.swap(*this);
        other.clear();
        
        return *this;
    }

    /*
     * Iterators 
     */
    iterator begin() noexcept {
        return mutable_iterator(cbegin());
    }
    
    const_iterator begin() const noexcept {
        return cbegin();
    }
    
    const_iterator cbegin() const noexcept {
        if(empty()) {
            return cend();
        }
        
        if(m_root->is_hash_node()) {
            return const_iterator(nullptr, &m_root->as_hash_node());
        }
        
        const trie_node& tnode = most_left_trie_node(m_root->as_trie_node());
        if(tnode.m_value_node != nullptr) {
            return const_iterator(&tnode);
        }
        else {
            const hash_node& hnode = tnode.first_child().as_hash_node();
            return const_iterator(&tnode, &hnode);
        }
    }
    
    iterator end() noexcept {
        return iterator(nullptr, nullptr);
    }
    
    const_iterator end() const noexcept {
        return cend();
    }
    
    const_iterator cend() const noexcept {
        return const_iterator(nullptr, nullptr);
    }
    
    
    /*
     * Capacity
     */
    bool empty() const noexcept {
        return m_nb_elements == 0;
    }
    
    size_type size() const noexcept {
        return m_nb_elements;
    }
    
    size_type max_size() const noexcept {
        return std::numeric_limits<size_type>::max();
    }
    
    size_type max_key_size() const noexcept {
        return array_hash::MAX_KEY_SIZE;
    }
    
    
    /*
     * Modifiers
     */
    void clear() noexcept {
        m_root.reset(nullptr);
        m_nb_elements = 0;
    }
    
    template<class... ValueArgs>
    std::pair<iterator, bool> insert(const CharT* key, size_type key_size, ValueArgs&&... value_args) {
        if(key_size > max_key_size()) {
            throw std::length_error("Key is too long.");
        }
        
        if(m_root == nullptr) {
            m_root.reset(new hash_node(m_hash, m_max_load_factor));
        }
        
        return insert_impl(*m_root, key, key_size, std::forward<ValueArgs>(value_args)...); 
    }
    
    iterator erase(const_iterator pos) {
        return erase(mutable_iterator(pos));
    }
    
    iterator erase(const_iterator first, const_iterator last) {
        if(first == last) {
            return mutable_iterator(first);
        }
        
        auto to_delete = erase(first);
        while(to_delete != last) {
            to_delete = erase(to_delete);
        }
        
        return to_delete;
    }
    
    size_type erase(const CharT* key, size_type key_size) {
        auto it = find(key, key_size);
        if(it == end()) {
            return 0;
        }
        else {
            erase(it);
            return 1;
        }
        
    }
    
    void swap(htrie_hash& other) {
        using std::swap;
        
        swap(m_hash, other.m_hash);
        swap(m_root, other.m_root);
        swap(m_nb_elements, other.m_nb_elements);
        swap(m_max_load_factor, other.m_max_load_factor);
    }
    
    /*
     * Lookup
     */
    template<class U = T, typename std::enable_if<has_value<U>::value>::type* = nullptr>
    U& at(const CharT* key, size_type key_size) {
        return const_cast<U&>(static_cast<const htrie_hash*>(this)->at(key, key_size));
    }
    
    template<class U = T, typename std::enable_if<has_value<U>::value>::type* = nullptr>
    const U& at(const CharT* key, size_type key_size) const {
        auto it_find = find(key, key_size);
        if(it_find != end()) {
            return it_find.value();
        }
        else {
            throw std::out_of_range("Couldn't find key.");
        }        
    }
    
    //TODO optimize
    template<class U = T, typename std::enable_if<has_value<U>::value>::type* = nullptr>
    U& access_operator(const CharT* key, size_type key_size) {
        auto it_find = find(key, key_size);
        if(it_find != end()) {
            return it_find.value();
        }
        else {
            return insert(key, key_size, U()).first.value();
        }
    }
    
    size_type count(const CharT* key, size_type key_size) const {
        if(find(key, key_size) != end()) {
            return 1;
        }
        else {
            return 0;
        }
    }
    
    iterator find(const CharT* key, size_type key_size) {
        if(m_root == nullptr) {
            return end();
        }
        
        return find_impl(*m_root, key, key_size);
    }
    
    const_iterator find(const CharT* key, size_type key_size) const {
        if(m_root == nullptr) {
            return cend();
        }
        
        return find_impl(*m_root, key, key_size);
    }
    
    std::pair<iterator, iterator> equal_range(const CharT* key, size_type key_size) {
        iterator it = find(key, key_size);
        return std::make_pair(it, it);
    }
    
    std::pair<const_iterator, const_iterator> equal_range(const CharT* key, size_type key_size) const {
        const_iterator it = find(key, key_size);
        return std::make_pair(it, it);
    }
    
    /*
     *  Hash policy 
     */
    float max_load_factor() const { 
        return m_max_load_factor; 
    }
    
    void max_load_factor(float ml) { 
        m_max_load_factor = ml; 
    }
    
    /*
     * Observers
     */
    hasher hash_function() const { 
        return m_hash; 
    }
    
private:
    template<class... ValueArgs>
    std::pair<iterator, bool> insert_impl(anode& search_start_node, 
                                          const CharT* key, size_type key_size, ValueArgs&&... value_args) 
    {
        trie_node* current_node_parent = nullptr;
        anode* current_node = &search_start_node;
        
        for(size_type ikey = 0; ikey < key_size; ikey++) {
            if(current_node->is_trie_node()) {
                trie_node* tnode = &current_node->as_trie_node();
                
                const std::size_t pos = as_position(key[ikey]);
                if(tnode->m_children[pos] == nullptr) {
                    std::unique_ptr<hash_node> hnode(new hash_node(m_hash, m_max_load_factor));
                    auto insert_it = hnode->m_array_hash.emplace_ks(key + ikey + 1, key_size - ikey - 1, 
                                                                    std::forward<ValueArgs>(value_args)...);
                    
                    hnode->m_child_of_char = key[ikey];
                    tnode->m_children[pos] = std::move(hnode);
                    m_nb_elements++;
                    
                    hash_node* hnode_ptr = &tnode->m_children[pos]->as_hash_node();
                    return std::make_pair(iterator(tnode, hnode_ptr, insert_it.first, hnode_ptr->m_array_hash.end()), true);
                }
                else {
                    current_node_parent = tnode;
                    current_node = tnode->m_children[pos].get();
                }
            }
            else {
                const char child_of_char = (current_node_parent != nullptr)?key[ikey-1]:0;
                return insert_in_hash_node(current_node->as_hash_node(), child_of_char, current_node_parent,
                                           key + ikey, key_size - ikey, std::forward<ValueArgs>(value_args)...);
            }
        }
        
        
        if(current_node->is_trie_node()) {
            trie_node* tnode = &current_node->as_trie_node();
            if(tnode->m_value_node != nullptr) {
                return std::make_pair(iterator(tnode), false);
            }
            else {
                tnode->m_value_node.reset(new value_node(std::forward<ValueArgs>(value_args)...));
                m_nb_elements++;
                
                return std::make_pair(iterator(tnode), true);
            }
        }
        else {
            const char child_of_char = (current_node_parent != nullptr)?key[key_size-1]:0;
            return insert_in_hash_node(current_node->as_hash_node(), child_of_char, current_node_parent,
                                       "", 0, std::forward<ValueArgs>(value_args)...);
        }
    }
    
    template<class... ValueArgs>
    std::pair<iterator, bool> insert_in_hash_node(hash_node& child, char child_of_char, trie_node* parent, 
                                                  const CharT* key, size_type key_size, ValueArgs&&... value_args)
    {
        if(need_burst(child)) {
            std::unique_ptr<trie_node> new_node = burst(child);
            if(parent == nullptr) {
                m_root = std::move(new_node);
                return insert_impl(*m_root, key, key_size, std::forward<ValueArgs>(value_args)...);
            }
            else {
                new_node->m_parent_node = parent;
                new_node->m_child_of_char = child_of_char;
                
                parent->m_children[as_position(child_of_char)] = std::move(new_node);
                
                return insert_impl(*parent->m_children[as_position(child_of_char)], 
                                   key, key_size, std::forward<ValueArgs>(value_args)...);
            }
        }
        
        
        
        auto it_insert = child.m_array_hash.emplace_ks(key, key_size, std::forward<ValueArgs>(value_args)...);
        if(!it_insert.second) {
            return std::make_pair(iterator(parent, &child, it_insert.first, child.m_array_hash.end()), false);
        }
        else {
            m_nb_elements++;
            return std::make_pair(iterator(parent, &child, it_insert.first, child.m_array_hash.end()), true);
        }
    }
    

    
    
    iterator erase(iterator pos) {
        iterator next_pos = std::next(pos);
        
        if(pos.m_read_trie_node_value) {
            tsl_assert(pos.m_current_trie_node != nullptr && pos.m_current_trie_node->m_value_node != nullptr);
            
            pos.m_current_trie_node->m_value_node.reset(nullptr);
            m_nb_elements--;
            
            return next_pos;
        }
        
        
        tsl_assert(pos.m_current_hash_node != nullptr);
        auto next_array_hash_it = pos.m_current_hash_node->m_array_hash.erase(pos.m_array_hash_iterator);
        m_nb_elements--;
        
        if(next_array_hash_it != pos.m_current_hash_node->m_array_hash.end()) {
            return iterator(pos.m_current_trie_node, pos.m_current_hash_node, 
                            next_array_hash_it, pos.m_current_hash_node->m_array_hash.end());
        }
        else {
            if(pos.m_current_hash_node->m_array_hash.empty()) {
                clear_empty_nodes(*pos.m_current_hash_node, pos.m_current_trie_node);
            }
                        
            return next_pos;
        }
    }
    
    void clear_empty_nodes(hash_node& empty_hnode, trie_node* parent) noexcept {
        tsl_assert(empty_hnode.m_array_hash.empty());
        
        if(parent == nullptr || parent->m_value_node != nullptr) {
            return;
        }
        
        if(!parent->empty()) {
            parent->m_children[as_position(empty_hnode.m_child_of_char)].reset(nullptr);
        }
        else if(parent->m_parent_node == nullptr) {
            m_root = std::move(parent->m_children[as_position(empty_hnode.m_child_of_char)]);
        }
        else {
            /**
             * Parent is empty. Put empty_hnode as new child of the grand parent instead of parent (move hnode up,
             * and delete the parent).
             */
            const char hnode_new_child_of_char = parent->m_child_of_char;
            
            trie_node* grand_parent = parent->m_parent_node;
            grand_parent->m_children[as_position(parent->m_child_of_char)] = 
                                        std::move(parent->m_children[as_position(empty_hnode.m_child_of_char)]);
            empty_hnode.m_child_of_char = hnode_new_child_of_char;
            
            
            
            clear_empty_nodes(empty_hnode, grand_parent);
        }
    }    
    
    
    
    
    iterator find_impl(const anode& search_start_node, const CharT* key, size_type key_size) {
        return mutable_iterator(static_cast<const htrie_hash*>(this)->find_impl(search_start_node, key, key_size));
    }
    
    const_iterator find_impl(const anode& search_start_node, const CharT* key, size_type key_size) const {
        const trie_node* current_node_parent = nullptr;
        const anode* current_node = &search_start_node;
        
        for(size_type ikey = 0; ikey < key_size; ikey++) {
            if(current_node->is_trie_node()) {
                const trie_node* tnode = &current_node->as_trie_node();
                
                const std::size_t pos = as_position(key[ikey]);
                if(tnode->m_children[pos] == nullptr) {
                    return cend();
                }
                else {
                    current_node_parent = tnode;
                    current_node = tnode->m_children[pos].get();
                }
            }
            else {
                return find_in_hash_node(current_node->as_hash_node(), current_node_parent, 
                                         key + ikey, key_size - ikey);
            }
        }
        
        
        if(current_node->is_trie_node()) {
            const trie_node* node = &current_node->as_trie_node();
            return (node->m_value_node != nullptr)?const_iterator(node):cend();
        }
        else {
            return find_in_hash_node(current_node->as_hash_node(), current_node_parent, "", 0);
        }
    }
    
    const_iterator find_in_hash_node(const hash_node& hnode, const trie_node* parent, 
                                     const CharT* key, size_type key_size) const 
    {
        auto it = hnode.m_array_hash.find_ks(key, key_size);
        if(it != hnode.m_array_hash.end()) {
            return const_iterator(parent, &hnode, it, hnode.m_array_hash.end());
        }
        else {
            return cend();
        }
    }
    
    
    
    
    /*
     * Burst
     */
    bool need_burst(hash_node& node) const {
        const size_type threshold = size_type(80/m_max_load_factor*
                                              HASH_NODE_DEFAULT_INIT_BUCKETS_COUNT*
                                              m_max_load_factor); // ~2500
        return node.m_array_hash.size() >= threshold;
    }
    
    
    template<class U = T, typename std::enable_if<has_value<U>::value && 
                                                  std::is_copy_constructible<U>::value && 
                                                  (!std::is_nothrow_move_constructible<U>::value ||
                                                   !std::is_nothrow_move_assignable<U>::value)>::type* = nullptr>
    std::unique_ptr<trie_node> burst(hash_node& node) {
        const std::array<size_type, ALPHABET_SIZE> first_char_count = get_first_char_count(node.m_array_hash.cbegin(), 
                                                                                           node.m_array_hash.cend());
        
        std::unique_ptr<trie_node> new_node(new trie_node());
        for(auto it = node.m_array_hash.cbegin(); it != node.m_array_hash.cend(); ++it) {
            if(it.key_size() == 0) {
                new_node->m_value_node.reset(new value_node(it.value()));
            }
            else {
                hash_node& hnode = get_hash_node_for_char(first_char_count, *new_node, it.key()[0]);
                hnode.m_array_hash.insert_ks(it.key()+1, it.key_size()-1, it.value());
            }
        }
        
        burst_children(*new_node);
        
        return new_node;
    } 
    
    template<class U = T, typename std::enable_if<has_value<U>::value && 
                                                  std::is_nothrow_move_constructible<U>::value &&
                                                  std::is_nothrow_move_assignable<U>::value>::type* = nullptr>
    std::unique_ptr<trie_node> burst(hash_node& node) {
        /**
         * We burst the node->m_array_hash into multiple arrays hash. While doing so, we move each value in 
         * the node->m_array_hash into the new arrays hash. After each move, we save a pointer to where the value
         * has been moved. In case of exception, we rollback these values into the original node->m_array_hash.
         */
        std::vector<T*> moved_values_rollback;
        moved_values_rollback.reserve(node.m_array_hash.size());
        
        try {
            const std::array<size_type, ALPHABET_SIZE> first_char_count = get_first_char_count(node.m_array_hash.cbegin(), 
                                                                                               node.m_array_hash.cend());
            
            std::unique_ptr<trie_node> new_node(new trie_node());
            for(auto it = node.m_array_hash.begin(); it != node.m_array_hash.end(); ++it) {
                if(it.key_size() == 0) {
                    new_node->m_value_node.reset(new value_node(std::move(it.value())));
                    moved_values_rollback.push_back(&new_node->m_value_node->m_value);
                }
                else {
                    hash_node& hnode = get_hash_node_for_char(first_char_count, *new_node, it.key()[0]);
                    auto it_insert = hnode.m_array_hash.insert_ks(it.key()+1, it.key_size()-1, std::move(it.value()));
                    moved_values_rollback.push_back(&it_insert.first.value());
                }
            }
            
            burst_children(*new_node);
            
            return new_node;
        }
        catch(...) {
            // Rollback the values
            auto it = node.m_array_hash.begin();
            for(std::size_t ivalue = 0; ivalue < moved_values_rollback.size(); ivalue++) {
                it.value() = std::move(*moved_values_rollback[ivalue]);
                
                ++it;
            }
            
            throw;
        }
    } 
    
    template<class U = T, typename std::enable_if<!has_value<U>::value>::type* = nullptr>
    std::unique_ptr<trie_node> burst(hash_node& node) {
        const std::array<size_type, ALPHABET_SIZE> first_char_count = get_first_char_count(node.m_array_hash.begin(), 
                                                                                   node.m_array_hash.end());
        
        std::unique_ptr<trie_node> new_node(new trie_node());
        for(auto it = node.m_array_hash.cbegin(); it != node.m_array_hash.cend(); ++it) {
            if(it.key_size() == 0) {
                new_node->m_value_node.reset(new value_node());
            }
            else {
                hash_node& hnode = get_hash_node_for_char(first_char_count, *new_node, it.key()[0]);
                hnode.m_array_hash.insert_ks(it.key()+1, it.key_size()-1);
            }
        }
        
        burst_children(*new_node);
        
        return new_node;
    }
    
    void burst_children(trie_node& node) {
        for(auto& child: node.m_children) {
            if(child == nullptr) {
                continue;
            }
            
            hash_node& child_hash_node = child->as_hash_node();
            if(need_burst(child_hash_node)) {
                std::unique_ptr<trie_node> new_child = burst(child_hash_node);
                new_child->m_parent_node = &node;
                new_child->m_child_of_char = child_hash_node.m_child_of_char;
                
                child = std::move(new_child);
            }
        }
    }
        
    std::array<size_type, ALPHABET_SIZE> get_first_char_count(typename array_hash::const_iterator begin, 
                                                              typename array_hash::const_iterator end) const
    {
        std::array<size_type, ALPHABET_SIZE> count = {};
        for(auto it = begin; it != end; ++it) {
            if(it.key_size() == 0) {
                continue;
            }
            
            count[as_position(it.key()[0])]++;
        }
        
        return count;
    }
    
    
    hash_node& get_hash_node_for_char(const std::array<size_type, ALPHABET_SIZE>& first_char_count, 
                                      trie_node& tnode, char for_char)
    {
        const std::size_t pos = as_position(for_char);
        if(tnode.m_children[pos] == nullptr) {
            const size_type nb_buckets = 
                                size_type(std::ceil(
                                    float(first_char_count[pos] + HASH_NODE_DEFAULT_INIT_BUCKETS_COUNT/2)/
                                    m_max_load_factor
                                ));
            tnode.m_children[pos].reset(new hash_node(nb_buckets, m_hash, m_max_load_factor));
            tnode.m_children[pos]->m_child_of_char = for_char;
        }
        
        return tnode.m_children[pos]->as_hash_node();
    }
    
    
    
    
    iterator mutable_iterator(const_iterator it) noexcept {
        if(it.m_current_hash_node == nullptr) {
            typename array_hash::iterator default_it;
            
            return iterator(const_cast<trie_node*>(it.m_current_trie_node), nullptr,
                            default_it, default_it, it.m_read_trie_node_value);
        }
        else {
            hash_node* hnode = const_cast<hash_node*>(it.m_current_hash_node);
            return iterator(const_cast<trie_node*>(it.m_current_trie_node), hnode,
                            hnode->m_array_hash.mutable_iterator(it.m_array_hash_iterator),
                            hnode->m_array_hash.mutable_iterator(it.m_array_hash_end_iterator),
                            it.m_read_trie_node_value);
        }
    }
public:    
    static constexpr float HASH_NODE_DEFAULT_MAX_LOAD_FACTOR = 5.0f;
    
private:
    static const size_type HASH_NODE_DEFAULT_INIT_BUCKETS_COUNT = 32;
    
    std::unique_ptr<anode> m_root;
    size_type m_nb_elements;
    Hash m_hash;
    float m_max_load_factor;
    
};

} // end namespace detail_htrie_hash
} // end namespace tsl

#endif
