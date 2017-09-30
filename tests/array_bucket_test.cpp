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
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>

#include <tsl/array_hash.h>
#include "utils.h"

BOOST_AUTO_TEST_SUITE(test_array_bucket)
using test_types = 
    boost::mpl::list<
        tsl::detail_array_hash::array_bucket<char, std::uint64_t, tsl::ah::str_equal<char>, std::uint16_t, true>,
        tsl::detail_array_hash::array_bucket<wchar_t, std::uint32_t, tsl::ah::str_equal<wchar_t>, std::uint8_t, true>,
        tsl::detail_array_hash::array_bucket<char16_t, std::uint32_t, tsl::ah::str_equal<char16_t>, std::uint8_t, true>,
        tsl::detail_array_hash::array_bucket<char32_t, std::uint8_t, tsl::ah::str_equal<char32_t>, std::uint32_t, true>,
        tsl::detail_array_hash::array_bucket<char16_t, std::uint16_t, tsl::ah::str_equal<char16_t>, std::uint32_t, false>
    >;


/**
 * insert and erase
 */                                      
BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert, ABucket, test_types) {
    // insert x values, check values, remove half, check values, remove second half
    using char_tt = typename ABucket::char_type; 
    using mapped_tt = typename ABucket::mapped_type; 
    using key_equal = typename ABucket::key_equal;
    
    ABucket bucket;
    
    // insert `nb_values` values
    const std::size_t nb_values = 1000;
    for(std::size_t i = 0; i < nb_values; i++) {
        const auto key = utils::get_key<char_tt>(i);
        
        auto it_find = bucket.find_or_end_of_bucket(key.data(), key.size());
        BOOST_REQUIRE(!it_find.second);
        
        auto it_insert = bucket.append(it_find.first, key.data(), key.size(), mapped_tt(i));
        BOOST_CHECK(key_equal()(it_insert.key(), it_insert.key_size(), key.data(), key.size()));
        
        BOOST_CHECK_EQUAL(std::distance(bucket.begin(), bucket.end()), i + 1);
        BOOST_CHECK_EQUAL(std::distance(bucket.cbegin(), bucket.cend()), i + 1);
    }
    
    // Check values
    std::size_t i = 0;
    for(auto it = bucket.begin(); it != bucket.end(); ++it) {
        const auto key = utils::get_key<char_tt>(i);
        BOOST_CHECK(key_equal()(it.key(), it.key_size(), key.data(), key.size()));
        BOOST_CHECK_EQUAL(it.value(), mapped_tt(i));
        i++;
    }
    
    // Remove half value
    for(std::size_t i = 0; i < nb_values; i += 2) {
        const auto key = utils::get_key<char_tt>(i);
        BOOST_CHECK(bucket.erase(key.data(), key.size()));
    }
    
        
    // Check values
    i = 1;
    for(auto it = bucket.begin(); it != bucket.end(); ++it) {
        const auto key = utils::get_key<char_tt>(i);
        BOOST_CHECK(key_equal()(it.key(), it.key_size(), key.data(), key.size()));
        BOOST_CHECK_EQUAL(it.value(), mapped_tt(i));
        i += 2;
    }
    BOOST_CHECK_EQUAL(std::distance(bucket.begin(), bucket.end()), nb_values/2);
    BOOST_CHECK_EQUAL(std::distance(bucket.cbegin(), bucket.cend()), nb_values/2);
    
    
    
    // Remove second half
    for(std::size_t i = 1; i < nb_values; i += 2) {
        const auto key = utils::get_key<char_tt>(i);
        BOOST_CHECK(bucket.erase(key.data(), key.size()));
    }
    
    BOOST_CHECK_EQUAL(std::distance(bucket.begin(), bucket.end()), 0);
    BOOST_CHECK_EQUAL(std::distance(bucket.cbegin(), bucket.cend()), 0);
    BOOST_CHECK(bucket.empty());
}

/**
 * append_in_reserved_bucket_no_check
 */
BOOST_AUTO_TEST_CASE_TEMPLATE(test_append_in_reserved, ABucket, test_types) {
    // get required size, insert x values, check values
    using char_tt = typename ABucket::char_type; 
    using mapped_tt = typename ABucket::mapped_type; 
    using key_equal = typename ABucket::key_equal;
    
    ABucket bucket;
    
    const std::size_t nb_values = 1000;
    
    std::size_t required_size = 0;
    for(std::size_t i = 0; i < nb_values; i++) {
        required_size += ABucket::entry_required_bytes(utils::get_key<char_tt>(i).size());
    }
    
    bucket.reserve(required_size);
    
    for(std::size_t i = 0; i < nb_values; i++) {
        const auto key = utils::get_key<char_tt>(i);
        bucket.append_in_reserved_bucket_no_check(key.data(), key.size(), mapped_tt(i));
        
        BOOST_CHECK_EQUAL(std::distance(bucket.begin(), bucket.end()), i + 1);
    }
    
    for(std::size_t i = 0; i < nb_values; i++) {
        const auto key = utils::get_key<char_tt>(i);
        auto it_find = bucket.find_or_end_of_bucket(key.data(), key.size());
        
        BOOST_REQUIRE(it_find.second);
        BOOST_CHECK(key_equal()(it_find.first.key(), it_find.first.key_size(), key.data(), key.size()));
        BOOST_CHECK_EQUAL(it_find.first.value(), mapped_tt(i));
    }
}

/**
 * erase
 */
BOOST_AUTO_TEST_CASE_TEMPLATE(test_erase_all, ABucket, test_types) {
    // insert x values, erase all values one by one
    using char_tt = typename ABucket::char_type; 
    using mapped_tt = typename ABucket::mapped_type; 
    using key_equal = typename ABucket::key_equal;
    
    ABucket bucket;
    
    // insert `nb_values` values
    const std::size_t nb_values = 1000;
    for(std::size_t i = 0; i < nb_values; i++) {
        const auto key = utils::get_key<char_tt>(i);
        
        auto it_find = bucket.find_or_end_of_bucket(key.data(), key.size());
        BOOST_REQUIRE(!it_find.second);
        
        auto it_insert = bucket.append(it_find.first, key.data(), key.size(), mapped_tt(i));
        BOOST_CHECK(key_equal()(it_insert.key(), it_insert.key_size(), key.data(), key.size()));
    }    
    
    // erase all values
    auto it_erase = bucket.cbegin();
    while(it_erase != bucket.cend()) {
        it_erase = bucket.erase(it_erase);
    }
    
    BOOST_CHECK_EQUAL(std::distance(bucket.begin(), bucket.end()), 0);
    BOOST_CHECK_EQUAL(std::distance(bucket.cbegin(), bucket.cend()), 0);
    BOOST_CHECK(bucket.empty());
}


/**
 * iterator
 */
BOOST_AUTO_TEST_CASE(test_iterator) {
    using ABucket = tsl::detail_array_hash::array_bucket<char, void, tsl::ah::str_equal<char>, std::uint16_t, true>;
    ABucket bucket;
    
    BOOST_CHECK(bucket.empty());
    BOOST_CHECK_EQUAL(std::distance(bucket.begin(), bucket.end()), 0);
    BOOST_CHECK_EQUAL(std::distance(bucket.cbegin(), bucket.cend()), 0);
    
    BOOST_CHECK(bucket.end() == ABucket::end_it());
    BOOST_CHECK(bucket.cend() == ABucket::cend_it());
    
}

/**
 * KeyEqual
 */
BOOST_AUTO_TEST_CASE(test_key_equal) {
    // insert x values using case-insensitive KeyEqual, check values with different case
    using ABucket = tsl::detail_array_hash::array_bucket<wchar_t, void, ci_str_equal<wchar_t>, std::uint8_t, false>;
    using key_equal = typename ABucket::key_equal;
    
    ABucket bucket;
    
    const std::size_t nb_values = 1000;
    for(std::size_t i = 0; i < nb_values; i++) {
        const std::wstring key = L"KEy " + std::to_wstring(i);
        
        auto it_find = bucket.find_or_end_of_bucket(key.data(), key.size());
        BOOST_REQUIRE(!it_find.second);
        
        auto it_insert = bucket.append(it_find.first, key.data(), key.size());
        BOOST_CHECK(key_equal()(it_insert.key(), it_insert.key_size(), key.data(), key.size()));
    }
    
    
    for(std::size_t i = 0; i < nb_values; i++) {
        const std::wstring key = L"kEY " + std::to_wstring(i);
        auto it_find = bucket.find_or_end_of_bucket(key.data(), key.size());
        
        BOOST_REQUIRE(it_find.second);
        BOOST_CHECK(key_equal()(it_find.first.key(), it_find.first.key_size(), key.data(), key.size()));
    }
}


BOOST_AUTO_TEST_SUITE_END()
