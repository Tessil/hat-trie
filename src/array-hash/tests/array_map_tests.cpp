#define BOOST_TEST_DYN_LINK

#include <cstdint>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>

#include "array_map.h"
#include "utils.h"


using test_types = boost::mpl::list<
                tsl::array_map<char, int64_t>,
                tsl::array_map<char, std::string>,
                tsl::array_map<char, move_only_test>,
                tsl::array_map<wchar_t, move_only_test>,
                tsl::array_map<char16_t, move_only_test>,
                tsl::array_map<char32_t, move_only_test>,
                tsl::array_map<char, move_only_test, tsl::str_hash<char>, std::char_traits<char>, false>,
                tsl::array_map<wchar_t, move_only_test, tsl::str_hash<wchar_t>, std::char_traits<wchar_t>, false>,
                tsl::array_map<char16_t, move_only_test, tsl::str_hash<char16_t>, std::char_traits<char16_t>, false>,
                tsl::array_map<char32_t, move_only_test, tsl::str_hash<char32_t>, std::char_traits<char32_t>, false>
                >;


/**
 * insert
 */                                      
BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert, AMap, test_types) {
    // insert x values, insert them again, check values
    using char_tt = typename AMap::char_type; 
    using traits_tt = typename AMap::traits_type; 
    using value_tt = typename AMap::mapped_type;
    
    
    const size_t nb_values = 1000;
    AMap map;
    typename AMap::iterator it;
    bool inserted;
    
    for(size_t i = 0; i < nb_values; i++) {
        std::tie(it, inserted) = map.insert(utils::get_key<char_tt, traits_tt>(i), utils::get_value<value_tt>(i));
        
        BOOST_CHECK_EQUAL(it.key_size(), (utils::get_key<char_tt, traits_tt>(i)).size());
        BOOST_CHECK(traits_tt::compare(it.key(), utils::get_key<char_tt, traits_tt>(i).c_str(), it.key_size()) == 0);
        BOOST_CHECK_EQUAL(*it, utils::get_value<value_tt>(i));
        BOOST_CHECK(inserted);
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values);
    
    for(size_t i = 0; i < nb_values; i++) {
        std::tie(it, inserted) = map.insert(utils::get_key<char_tt, traits_tt>(i), utils::get_value<value_tt>(i+1));
        
        BOOST_CHECK_EQUAL(it.key_size(), (utils::get_key<char_tt, traits_tt>(i)).size());
        BOOST_CHECK(traits_tt::compare(it.key(), utils::get_key<char_tt, traits_tt>(i).c_str(), it.key_size()) == 0);
        BOOST_CHECK_EQUAL(*it, utils::get_value<value_tt>(i));
        BOOST_CHECK(!inserted);
    }
    
    for(size_t i = 0; i < nb_values; i++) {
        it = map.find(utils::get_key<char_tt, traits_tt>(i));
        
        BOOST_CHECK(it != map.end());
        BOOST_CHECK_EQUAL(it.key_size(), (utils::get_key<char_tt, traits_tt>(i)).size());
        BOOST_CHECK(traits_tt::compare(it.key(), utils::get_key<char_tt, traits_tt>(i).c_str(), it.key_size()) == 0);
        BOOST_CHECK_EQUAL(*it, utils::get_value<value_tt>(i));
    }
}

BOOST_AUTO_TEST_CASE(test_insert_more_than_max_size) {
    tsl::array_map<char, int64_t, tsl::str_hash<char>, std::char_traits<char>, true, 
                   std::uint16_t, std::uint8_t> map;
    for(std::size_t i=0; i < map.max_size(); i++) {
        map.insert(utils::get_key<char, std::char_traits<char>>(i), utils::get_value<int64_t>(i));
    }
    
    BOOST_CHECK_EQUAL(map.size(), map.max_size());
    BOOST_CHECK_THROW(map.insert(utils::get_key<char, std::char_traits<char>>(map.max_size()), 
                                 utils::get_value<int64_t>(map.max_size())), 
                      std::length_error);
}

BOOST_AUTO_TEST_CASE(test_insert_more_than_max_size_with_erase) {
    tsl::array_map<char, int64_t, tsl::str_hash<char>, std::char_traits<char>, true, 
                   std::uint16_t, std::uint8_t> map;
    for(std::size_t i=0; i < map.max_size(); i++) {
        map.insert(utils::get_key<char, std::char_traits<char>>(i), utils::get_value<int64_t>(i));
    }
    map.erase(utils::get_key<char, std::char_traits<char>>(0));
    
    BOOST_CHECK_EQUAL(map.size(), map.max_size()-1);
    BOOST_CHECK_THROW(map.insert(utils::get_key<char, std::char_traits<char>>(map.max_size()), 
                                 utils::get_value<int64_t>(map.max_size())), 
                      std::length_error);
}

BOOST_AUTO_TEST_CASE(test_insert_with_too_long_string) {
    tsl::array_map<char, int64_t, tsl::str_hash<char>, std::char_traits<char>, true,
                   std::uint8_t, std::uint16_t> map;
    for(std::size_t i=0; i < 10; i++) {
        map.insert(utils::get_key<char, std::char_traits<char>>(i), utils::get_value<int64_t>(i));
    }
    
    const std::string long_string("a", map.max_key_size());
    BOOST_CHECK(map.insert(long_string, utils::get_value<int64_t>(10)).second);
    
    const std::string too_long_string("a", map.max_key_size() + 1);
    BOOST_CHECK_THROW(map.insert(too_long_string, utils::get_value<int64_t>(11)), std::length_error);
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
    using traits_tt = typename AMap::traits_type; 
    
    size_t nb_values = 1000;
    AMap map = utils::get_filled_map<AMap>(nb_values);
    
    BOOST_CHECK_EQUAL(map.erase(utils::get_key<char_tt, traits_tt>(1001)), 0);
    BOOST_CHECK(map.erase(map.cbegin(), map.cbegin()) == map.begin());
    BOOST_CHECK(map == utils::get_filled_map<AMap>(nb_values));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert_erase_insert, AMap, test_types) {
    // insert x/2 values, delete x/4 values, insert x/2 values, find each value
    using char_tt = typename AMap::char_type; 
    using traits_tt = typename AMap::traits_type; 
    using value_tt = typename AMap::mapped_type;
    
    const size_t nb_values = 2000;
    AMap map;
    typename AMap::iterator it;
    bool inserted;
    
    for(size_t i = 0; i < nb_values/2; i++) {
        std::tie(it, inserted) = map.insert(utils::get_key<char_tt, traits_tt>(i), utils::get_value<value_tt>(i));
        
        BOOST_CHECK_EQUAL(it.key_size(), (utils::get_key<char_tt, traits_tt>(i)).size());
        BOOST_CHECK(traits_tt::compare(it.key(), utils::get_key<char_tt, traits_tt>(i).c_str(), it.key_size()) == 0);
        BOOST_CHECK_EQUAL(*it, utils::get_value<value_tt>(i));
        BOOST_CHECK(inserted);
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values/2);
    
    
    // Delete half
    for(size_t i = 0; i < nb_values/2; i++) {
        if(i%2 == 0) {
            BOOST_CHECK_EQUAL(map.erase(utils::get_key<char_tt, traits_tt>(i)), 1);
            BOOST_CHECK(map.find(utils::get_key<char_tt, traits_tt>(i)) == map.end());
        }
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values/4);
    
    
    for(size_t i = nb_values/2; i < nb_values; i++) {
        std::tie(it, inserted) = map.insert(utils::get_key<char_tt, traits_tt>(i), utils::get_value<value_tt>(i));
        
        BOOST_CHECK_EQUAL(it.key_size(), (utils::get_key<char_tt, traits_tt>(i)).size());
        BOOST_CHECK(traits_tt::compare(it.key(), utils::get_key<char_tt, traits_tt>(i).c_str(), it.key_size()) == 0);
        BOOST_CHECK_EQUAL(*it, utils::get_value<value_tt>(i));
        BOOST_CHECK(inserted);
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values-nb_values/4);
    
    for(size_t i = 0; i < nb_values; i++) {
        it = map.find(utils::get_key<char_tt, traits_tt>(i));
            
        if(i%2 == 0 && i < nb_values/2) {
            BOOST_CHECK(it == map.cend());
        }
        else {
            BOOST_CHECK_EQUAL(it.key_size(), (utils::get_key<char_tt, traits_tt>(i)).size());
            BOOST_CHECK(traits_tt::compare(it.key(), utils::get_key<char_tt, traits_tt>(i).c_str(), it.key_size()) == 0);
            BOOST_CHECK_EQUAL(*it, utils::get_value<value_tt>(i));
        }
    }
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
 * iterator
 */
BOOST_AUTO_TEST_CASE(test_iterator_empty_map) {
    tsl::array_map<char, int64_t> map;
    BOOST_CHECK(map.begin() == map.end());
    BOOST_CHECK(map.begin() == map.cend());
    BOOST_CHECK(map.cbegin() == map.cend());
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
 * operator[]
 */
BOOST_AUTO_TEST_CASE(test_access_operator) {
    // insert x values, use at for known and unknown values.
    tsl::array_map<char, int64_t> map = {{"test1", 10}, {"test2", 20}};
    
    BOOST_CHECK_EQUAL(map["test1"], 10);
    BOOST_CHECK_EQUAL(map["test2"], 20);
    BOOST_CHECK_EQUAL(map["test3"], int64_t());
    
    BOOST_CHECK_EQUAL(map.size(), 3);
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
    tsl::array_map<char, int64_t, ci_str_hash<char>, ci_char_traits<char>> map = {{"test1", 10}, {"TeSt2", 20},
                                                                                  {"tesT3", 30}, {"test4", 40}, 
                                                                                  {"TEST5", 50}};
                                                                                  
    BOOST_CHECK_EQUAL(map.at("TEST1"), 10);        
    
    BOOST_CHECK(map.find("TeST3") != map.end());                     
    BOOST_CHECK_EQUAL(map.find("TeST3").value(), 30);
    
    BOOST_CHECK_EQUAL(map.at("test2"), 20);
    BOOST_CHECK_EQUAL(map.at("test4"), 40);  
    BOOST_CHECK_EQUAL(map.at("tEst5"), 50);    
}
