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

#include <cstdint>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>

#include "tsl/array_map.h"
#include "utils.h"

BOOST_AUTO_TEST_SUITE(test_array_map)

using test_types = 
    boost::mpl::list<
        tsl::array_map<char, int64_t>,
        tsl::array_map<char, std::string>,
        tsl::array_map<char, move_only_test>,
        tsl::array_map<wchar_t, move_only_test>,
        tsl::array_map<char16_t, move_only_test>,
        tsl::array_map<char32_t, move_only_test>,
        tsl::array_map<char, move_only_test, tsl::str_hash_ah<char>, tsl::str_equal_ah<char>, false>,
        tsl::array_map<wchar_t, move_only_test, tsl::str_hash_ah<wchar_t>, tsl::str_equal_ah<wchar_t>, false>,
        tsl::array_map<char16_t, move_only_test, tsl::str_hash_ah<char16_t>, tsl::str_equal_ah<char16_t>, false>,
        tsl::array_map<char32_t, move_only_test, tsl::str_hash_ah<char32_t>, tsl::str_equal_ah<char32_t>, false>,
        tsl::array_pg_map<char16_t, move_only_test>,
        tsl::array_map<char16_t, move_only_test, tsl::str_hash_ah<char16_t>, tsl::str_equal_ah<char16_t>, false,
                       std::uint16_t, std::uint16_t, tsl::mod_growth_policy_ah<>>
    >;


/**
 * insert
 */                                      
BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert, AMap, test_types) {
    // insert x values, insert them again, check values
    using char_tt = typename AMap::char_type; 
    using value_tt = typename AMap::mapped_type;
    
    
    const size_t nb_values = 1000;
    AMap map;
    typename AMap::iterator it;
    bool inserted;
    
    for(size_t i = 0; i < nb_values; i++) {
        const auto key = utils::get_key<char_tt>(i);
        std::tie(it, inserted) = map.insert(key, utils::get_value<value_tt>(i));
        
        BOOST_CHECK(map.key_eq()(it.key(), it.key_size(), key.c_str(), key.size()));
        BOOST_CHECK_EQUAL(*it, utils::get_value<value_tt>(i));
        BOOST_CHECK(inserted);
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values);
    
    for(size_t i = 0; i < nb_values; i++) {
        const auto key = utils::get_key<char_tt>(i);
        std::tie(it, inserted) = map.insert(key, utils::get_value<value_tt>(i+1));
        
        BOOST_CHECK(map.key_eq()(it.key(), it.key_size(), key.c_str(), key.size()));
        BOOST_CHECK_EQUAL(*it, utils::get_value<value_tt>(i));
        BOOST_CHECK(!inserted);
    }
    
    for(size_t i = 0; i < nb_values; i++) {
        const auto key = utils::get_key<char_tt>(i);
        it = map.find(key);
        
        BOOST_CHECK(it != map.end());
        BOOST_CHECK(map.key_eq()(it.key(), it.key_size(), key.c_str(), key.size()));
        BOOST_CHECK_EQUAL(*it, utils::get_value<value_tt>(i));
    }
}

BOOST_AUTO_TEST_CASE(test_insert_more_than_max_size) {
    tsl::array_map<char, int64_t, tsl::str_hash_ah<char>, tsl::str_equal_ah<char>, true, 
                   std::uint16_t, std::uint8_t> map;
    for(std::size_t i=0; i < map.max_size(); i++) {
        map.insert(utils::get_key<char>(i), utils::get_value<int64_t>(i));
    }
    
    BOOST_CHECK_EQUAL(map.size(), map.max_size());
    BOOST_CHECK_THROW(map.insert(utils::get_key<char>(map.max_size()), 
                                 utils::get_value<int64_t>(map.max_size())), 
                      std::length_error);
}

BOOST_AUTO_TEST_CASE(test_insert_with_too_long_string) {
    tsl::array_map<char, int64_t, tsl::str_hash_ah<char>, tsl::str_equal_ah<char>, true,
                   std::uint8_t, std::uint16_t> map;
    for(std::size_t i=0; i < 10; i++) {
        map.insert(utils::get_key<char>(i), utils::get_value<int64_t>(i));
    }
    
    const std::string long_string("a", map.max_key_size());
    BOOST_CHECK(map.insert(long_string, utils::get_value<int64_t>(10)).second);
    
    const std::string too_long_string("a", map.max_key_size() + 1);
    BOOST_CHECK_THROW(map.insert(too_long_string, utils::get_value<int64_t>(11)), std::length_error);
}


BOOST_AUTO_TEST_CASE(test_range_insert) {
    // insert x values in vector, range insert x-15 values from vector to map, check values
    const int nb_values = 1000;
    std::vector<std::pair<std::string, std::size_t>> values;
    for(std::size_t i = 0; i < nb_values; i++) {
        values.push_back(std::make_pair(utils::get_key<char>(i), i+1));
    }
    
    
    tsl::array_map<char, std::size_t> map = {{"range_key_1", 1}, {"range_key_2", 2}};
    map.insert(std::next(values.begin(), 10), std::prev(values.end(), 5));
    
    BOOST_CHECK_EQUAL(map.size(), 987);
    
    BOOST_CHECK_EQUAL(map["range_key_1"], 1);
    BOOST_CHECK_EQUAL(map["range_key_2"], 2);
    
    for(std::size_t i = 10; i < nb_values - 5; i++) {
        BOOST_CHECK_EQUAL(map[utils::get_key<char>(i)], i+1);
    }    
}



/**
 * insert_or_assign
 */
BOOST_AUTO_TEST_CASE(test_insert_or_assign) {
    tsl::array_map<char, move_only_test> map;
    tsl::array_map<char, move_only_test>::iterator it;    
    bool inserted;
    
    
    const std::string key1 = "key1";
    
    std::tie(it, inserted) = map.insert_or_assign(key1, move_only_test(1));
    BOOST_CHECK(map.key_eq()(it.key(), it.key_size(), key1.data(), key1.size()));
    BOOST_CHECK_EQUAL(it.value(), move_only_test(1));
    BOOST_CHECK(inserted);
    
    
    std::tie(it, inserted) = map.insert_or_assign("key1", move_only_test(3));
    BOOST_CHECK(map.key_eq()(it.key(), it.key_size(), key1.data(), key1.size()));
    BOOST_CHECK_EQUAL(it.value(), move_only_test(3));
    BOOST_CHECK(!inserted);
}

/**
 * emplace
 */
BOOST_AUTO_TEST_CASE(test_emplace) {
    tsl::array_map<char, move_only_test> map;
    map.emplace("test1", 1);
    map.emplace_ks("testIgnore", 4, 3);
    
    BOOST_CHECK_EQUAL(map.size(), 2);
    BOOST_CHECK(map.at("test1") ==  move_only_test(1));
    BOOST_CHECK(map.at("test") == move_only_test(3));
}

/**
 * erase
 */
BOOST_AUTO_TEST_CASE_TEMPLATE(test_erase_all, AMap, test_types) {
    // insert x values, delete all
    const size_t nb_values = 1000;
    AMap map = utils::get_filled_map<AMap>(nb_values);
    
    auto it = map.erase(map.begin(), map.end());
    BOOST_CHECK(it == map.end());
    BOOST_CHECK(map.empty());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_erase_loop, AMap, test_types) {
    // insert x values, delete all one by one
    size_t nb_values = 1000;
    AMap map = utils::get_filled_map<AMap>(nb_values);
    AMap map2 = utils::get_filled_map<AMap>(nb_values);
    
    auto it = map.begin();
    // Use second map to check for key after delete as we may not copy the key with move-only types.
    auto it2 = map2.begin();
    while(it != map.end()) {
        it = map.erase(it);
        --nb_values;
        
        BOOST_CHECK_EQUAL(map.count_ks(it2.key(), it2.key_size()), 0);
        BOOST_CHECK_EQUAL(map.size(), nb_values);
        ++it2;
    }
    
    BOOST_CHECK(map.empty());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_erase_unknown, AMap, test_types) {
    using char_tt = typename AMap::char_type;  
    
    size_t nb_values = 1000;
    AMap map = utils::get_filled_map<AMap>(nb_values);
    
    BOOST_CHECK_EQUAL(map.erase(utils::get_key<char_tt>(1001)), 0);
    BOOST_CHECK(map.erase(map.cbegin(), map.cbegin()) == map.begin());
    BOOST_CHECK(map == utils::get_filled_map<AMap>(nb_values));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert_erase_insert, AMap, test_types) {
    // insert x/2 values, delete x/4 values, insert x/2 values, find each value
    using char_tt = typename AMap::char_type; 
    using value_tt = typename AMap::mapped_type;
    
    const size_t nb_values = 2000;
    AMap map;
    typename AMap::iterator it;
    bool inserted;
    
    for(size_t i = 0; i < nb_values/2; i++) {
        const auto key = utils::get_key<char_tt>(i);
        std::tie(it, inserted) = map.insert(key, utils::get_value<value_tt>(i));
        
        BOOST_CHECK(map.key_eq()(it.key(), it.key_size(), key.c_str(), key.size()));
        BOOST_CHECK_EQUAL(*it, utils::get_value<value_tt>(i));
        BOOST_CHECK(inserted);
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values/2);
    
    
    // Delete half
    for(size_t i = 0; i < nb_values/2; i++) {
        if(i%2 == 0) {
            const auto key = utils::get_key<char_tt>(i);
            BOOST_CHECK_EQUAL(map.erase(key), 1);
            BOOST_CHECK(map.find(key) == map.end());
        }
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values/4);
    
    // Insert other half
    for(size_t i = nb_values/2; i < nb_values; i++) {
        const auto key = utils::get_key<char_tt>(i);
        std::tie(it, inserted) = map.insert(key, utils::get_value<value_tt>(i));
        
        BOOST_CHECK(map.key_eq()(it.key(), it.key_size(), key.c_str(), key.size()));
        BOOST_CHECK_EQUAL(*it, utils::get_value<value_tt>(i));
        BOOST_CHECK(inserted);
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values-nb_values/4);
    
    // Check values
    for(size_t i = 0; i < nb_values; i++) {
        const auto key = utils::get_key<char_tt>(i);
        it = map.find(key);
            
        if(i%2 == 0 && i < nb_values/2) {
            BOOST_CHECK(it == map.cend());
        }
        else {
            BOOST_CHECK(map.key_eq()(it.key(), it.key_size(), key.c_str(), key.size()));
            BOOST_CHECK_EQUAL(*it, utils::get_value<value_tt>(i));
        }
    }
}

/**
 * operator== and operator!=
 */
BOOST_AUTO_TEST_CASE(test_compare) {
    tsl::array_map<char, int64_t> map =  {{"test1", 10}, {"test2", 20}, {"test3", 30}};
    tsl::array_map<char, int64_t> map2 = {{"test3", 30}, {"test2", 20}, {"test1", 10}};
    tsl::array_map<char, int64_t> map3 = {{"test1", 10}, {"test2", 20}, {"test3", -1}};
    tsl::array_map<char, int64_t> map4 = {{"test3", 30}, {"test2", 20}};
    
    BOOST_CHECK(map == map);
    BOOST_CHECK(map2 == map2);
    BOOST_CHECK(map3 == map3);
    BOOST_CHECK(map4 == map4);
    
    BOOST_CHECK(map == map2);
    BOOST_CHECK(map != map3);
    BOOST_CHECK(map != map4);
    BOOST_CHECK(map2 != map3);
    BOOST_CHECK(map2 != map4);
    BOOST_CHECK(map3 != map4);
}


/**
 * clear
 */
BOOST_AUTO_TEST_CASE(test_clear) {
    tsl::array_map<char, int64_t> map = {{"test1", 10}, {"test2", 20}};
    
    map.clear();
    BOOST_CHECK_EQUAL(map.size(), 0);
    BOOST_CHECK(map.begin() == map.end());
    BOOST_CHECK(map.cbegin() == map.cend());
    
    map.insert("test3", 30);
    map.insert({{"test4", 40}, {"test5", 50}, {"test6", 60}, {"test7", 70}});
    
    BOOST_CHECK(map == (
                        tsl::array_map<char, int64_t>({{"test3", 30}, {"test4", 40}, {"test5", 50}, 
                                                       {"test6", 60}, {"test7", 70}})
                ));
}


/**
 * shrink_to_fit
 */
BOOST_AUTO_TEST_CASE(test_shrink_to_fit) {
    tsl::array_map<char, int64_t> map = utils::get_filled_map<tsl::array_map<char, int64_t>>(1000);
    tsl::array_map<char, int64_t> map2 = map;
    BOOST_CHECK(map == map2);
    
    auto it = map.begin();
    auto it2 = map2.begin();
    for(std::size_t i = 0; i < 200; i++) {
         it = map.erase(it);
        it2 = map2.erase(it2);
    }
    
    map.shrink_to_fit();
    BOOST_CHECK(map == map2);
    
    
    map.insert({{"shrink_test1", 10}, {"shrink_test2", 20}, {"shrink_test3", 30}, {"shrink_test4", 40}});
    map2.insert({{"shrink_test1", 10}, {"shrink_test2", 20}, {"shrink_test3", 30}, {"shrink_test4", 40}});
    BOOST_CHECK(map == map2);
}

/**
 * operator=
 */
BOOST_AUTO_TEST_CASE(test_assign_operator) {
    tsl::array_map<char, int64_t> map = {{"test1", 10}, {"test2", 20}};
    BOOST_CHECK_EQUAL(map.size(), 2);
    
    map = {{"test3", 30}};
    BOOST_CHECK_EQUAL(map.size(), 1);
    BOOST_CHECK_EQUAL(map.at("test3"), 30);
}


BOOST_AUTO_TEST_CASE(test_copy) {
    tsl::array_map<char, int64_t> map = {{"test1", 10}, {"test2", 20}, {"test3", 30}, {"test4", 40}};
    tsl::array_map<char, int64_t> map2 = map;
    tsl::array_map<char, int64_t> map3;
    map3 = map;
    
    BOOST_CHECK(map == map2);
    BOOST_CHECK(map == map3);
}


BOOST_AUTO_TEST_CASE(test_move) {
    tsl::array_map<char, int64_t> map = {{"test1", 10}, {"test2", 20}};
    tsl::array_map<char, int64_t> map2 = std::move(map);
    
    BOOST_CHECK(map.empty());
    BOOST_CHECK(map.begin() == map.end());
    BOOST_CHECK_EQUAL(map2.size(), 2);
    BOOST_CHECK(map2 == (tsl::array_map<char, int64_t>{{"test1", 10}, {"test2", 20}}));
    
    
    tsl::array_map<char, int64_t> map3;
    map3 = std::move(map2);
    
    BOOST_CHECK(map2.empty());
    BOOST_CHECK(map2.begin() == map2.end());
    BOOST_CHECK_EQUAL(map3.size(), 2);
    BOOST_CHECK(map3 == (tsl::array_map<char, int64_t>{{"test1", 10}, {"test2", 20}}));
    
    map2 = {{"test1", 10}};
    BOOST_CHECK(map2 == (tsl::array_map<char, int64_t>{{"test1", 10}}));
}

/**
 * at
 */
BOOST_AUTO_TEST_CASE(test_at) {
    // insert x values, use at for known and unknown values.
    tsl::array_map<char, int64_t> map = {{"test1", 10}, {"test2", 20}};
    map.insert("test4", 40);
    
    BOOST_CHECK_EQUAL(map.at("test1"), 10);
    BOOST_CHECK_EQUAL(map.at("test2"), 20);
    BOOST_CHECK_THROW(map.at("test3"), std::out_of_range);
    BOOST_CHECK_EQUAL(map.at("test4"), 40);
    
    
    const tsl::array_map<char, int64_t> map_const = {{"test1", 10}, {"test2", 20}, {"test4", 40}};
    
    BOOST_CHECK_EQUAL(map_const.at("test1"), 10);
    BOOST_CHECK_EQUAL(map_const.at("test2"), 20);
    BOOST_CHECK_THROW(map_const.at("test3"), std::out_of_range);
    BOOST_CHECK_EQUAL(map_const.at("test4"), 40);
}


/**
 * equal_range
 */
BOOST_AUTO_TEST_CASE(test_equal_range) {
    tsl::array_map<char, int64_t> map = {{"test1", 10}, {"test2", 20}};
    
    auto it_pair = map.equal_range("test1");
    BOOST_REQUIRE_EQUAL(std::distance(it_pair.first, it_pair.second), 1);
    BOOST_CHECK_EQUAL(it_pair.first.value(), 10);
    
    it_pair = map.equal_range("");
    BOOST_CHECK(it_pair.first == it_pair.second);
    BOOST_CHECK(it_pair.first == map.end());
}




/**
 * operator[]
 */
BOOST_AUTO_TEST_CASE(test_access_operator) {
    tsl::array_map<char, int64_t> map = {{"test1", 10}, {"test2", 20}};
    
    BOOST_CHECK_EQUAL(map["test1"], 10);
    BOOST_CHECK_EQUAL(map["test2"], 20);
    BOOST_CHECK_EQUAL(map["test3"], int64_t());
    
    BOOST_CHECK_EQUAL(map.size(), 3);
}

BOOST_AUTO_TEST_CASE(test_access_operator_2) {
    // insert x values, use at for known and unknown values.
    tsl::array_map<char, int64_t> map;
    
    std::size_t nb_values = 200;
    for(std::size_t i = 0; i < nb_values; i++) {
        map[utils::get_key<char>(i)] = int64_t(i);
    }
    
    for(std::size_t i = 0; i < nb_values; i++) {
        BOOST_CHECK_EQUAL(map[utils::get_key<char>(i)], int64_t(i));
    }
    
    BOOST_CHECK_EQUAL(map.size(), nb_values);
}

/**
 * swap
 */
BOOST_AUTO_TEST_CASE(test_swap) {
    tsl::array_map<char, int64_t> map = {{"test1", 10}, {"test2", 20}};
    tsl::array_map<char, int64_t> map2 = {{"test3", 30}, {"test4", 40}, {"test5", 50}};
    
    using std::swap;
    swap(map, map2);
    
    BOOST_CHECK(map == (tsl::array_map<char, int64_t>{{"test3", 30}, {"test4", 40}, {"test5", 50}}));
    BOOST_CHECK(map2 == (tsl::array_map<char, int64_t>{{"test1", 10}, {"test2", 20}}));
}

/**
 * Custom Traits, Custom Hash
 */
BOOST_AUTO_TEST_CASE(test_ci_traits) {
    tsl::array_map<char, int64_t, ci_str_hash<char>, ci_str_equal<char>> map = {{"test1", 10}, {"TeSt2", 20},
                                                                                {"tesT3", 30}, {"test4", 40}, 
                                                                                {"TEST5", 50}};
                                                                                  
    BOOST_CHECK_EQUAL(map.at("TEST1"), 10);        
    
    BOOST_CHECK(map.find("TeST3") != map.end());                     
    BOOST_CHECK_EQUAL(map.find("TeST3").value(), 30);
    
    BOOST_CHECK_EQUAL(map.at("test2"), 20);
    BOOST_CHECK_EQUAL(map.at("test4"), 40);  
    BOOST_CHECK_EQUAL(map.at("tEst5"), 50);    
}


/**
 * Various operations on empty map
 */
BOOST_AUTO_TEST_CASE(test_empty_map) {
    tsl::array_map<char, int> map(0);
    
    BOOST_CHECK_EQUAL(map.size(), 0);
    BOOST_CHECK(map.empty());
    
    BOOST_CHECK(map.begin() == map.end());
    BOOST_CHECK(map.begin() == map.cend());
    BOOST_CHECK(map.cbegin() == map.cend());
    
    BOOST_CHECK(map.find("") == map.end());
    BOOST_CHECK(map.find("test") == map.end());
    
    BOOST_CHECK_EQUAL(map.count(""), 0);
    BOOST_CHECK_EQUAL(map.count("test"), 0);
    
    BOOST_CHECK_THROW(map.at(""), std::out_of_range);
    BOOST_CHECK_THROW(map.at("test"), std::out_of_range);
    
    auto range = map.equal_range("test");
    BOOST_CHECK(range.first == range.second);
    
    BOOST_CHECK_EQUAL(map.erase("test"), 0);
    BOOST_CHECK(map.erase(map.begin(), map.end()) == map.end());
    
    BOOST_CHECK_EQUAL(map["new value"], int{});
}



/**
 * Test precalculated hash
 */
BOOST_AUTO_TEST_CASE(test_precalculated_hash) {
    tsl::array_map<char, int> map = {{"k1", -1}, {"k2", -2}, {"k3", -3}, {"k4", -4}, {"k5", -5}};
    const tsl::array_map<char, int> map_const = map;
    
    /**
     * find
     */
    BOOST_REQUIRE(map.find("k3", map.hash_function()("k3", strlen("k3"))) != map.end());
    BOOST_CHECK_EQUAL(map.find("k3", map.hash_function()("k3", strlen("k3"))).value(), -3);
    
    BOOST_REQUIRE(map_const.find("k3", map.hash_function()("k3", strlen("k3"))) != map_const.end());
    BOOST_CHECK_EQUAL(map_const.find("k3", map.hash_function()("k3", strlen("k3"))).value(), -3);
    
    BOOST_REQUIRE_NE(map.hash_function()("k2", strlen("k2")), map.hash_function()("k3", strlen("k3")));
    BOOST_CHECK(map.find("k3", map.hash_function()("k2", strlen("k2"))) == map.end());
    
    /**
     * at
     */
    BOOST_CHECK_EQUAL(map.at("k3", map.hash_function()("k3", strlen("k3"))), -3);
    BOOST_CHECK_EQUAL(map_const.at("k3", map_const.hash_function()("k3", strlen("k3"))), -3);
    
    BOOST_REQUIRE_NE(map.hash_function()("k2", strlen("k2")), map.hash_function()("k3", strlen("k3")));
    BOOST_CHECK_THROW(map.at("k3", map.hash_function()("k2", strlen("k2"))), std::out_of_range);
    
    /**
     * count
     */
    BOOST_CHECK_EQUAL(map.count("k3", map.hash_function()("k3", strlen("k3"))), 1);
    BOOST_CHECK_EQUAL(map_const.count("k3", map_const.hash_function()("k3", strlen("k3"))), 1);
    
    BOOST_REQUIRE_NE(map.hash_function()("k2", strlen("k2")), map.hash_function()("k3", strlen("k3")));
    BOOST_CHECK_EQUAL(map.count("k3", map.hash_function()("k2", strlen("k2"))), 0);
    
    /**
     * equal_range
     */
    auto it_range = map.equal_range("k3", map.hash_function()("k3", strlen("k3")));
    BOOST_REQUIRE_EQUAL(std::distance(it_range.first, it_range.second), 1);
    BOOST_CHECK_EQUAL(it_range.first.value(), -3);
    
    auto it_range_const = map_const.equal_range("k3", map_const.hash_function()("k3", strlen("k3")));
    BOOST_REQUIRE_EQUAL(std::distance(it_range_const.first, it_range_const.second), 1);
    BOOST_CHECK_EQUAL(it_range_const.first.value(), -3);
    
    it_range = map.equal_range("k3", map.hash_function()("k2", strlen("k2")));
    BOOST_REQUIRE_NE(map.hash_function()("k2", strlen("k2")), map.hash_function()("k3", strlen("k3")));
    BOOST_CHECK_EQUAL(std::distance(it_range.first, it_range.second), 0);
    
    /**
     * erase
     */
    BOOST_CHECK_EQUAL(map.erase("k3", map.hash_function()("k3", strlen("k3"))), 1);
    
    BOOST_REQUIRE_NE(map.hash_function()("k2", strlen("k2")), map.hash_function()("k4", strlen("k4")));
    BOOST_CHECK_EQUAL(map.erase("k4", map.hash_function()("k2", strlen("k2"))), 0);
}

BOOST_AUTO_TEST_SUITE_END()
