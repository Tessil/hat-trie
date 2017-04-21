#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>
#include <set>

#include "htrie_map.h"
#include "utils.h"


using test_types = boost::mpl::list<
                        tsl::htrie_map<char, int64_t>,
                        tsl::htrie_map<char, std::string>,
                        tsl::htrie_map<char, throw_move_test>,
                        tsl::htrie_map<char, move_only_test>
                        >;


/**
 * insert
 */                                      
BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert, TMap, test_types) {
    // insert x values, insert them again, check values
    using char_tt = typename TMap::char_type; 
    using value_tt = typename TMap::mapped_type;
    
    const size_t nb_values = 1000;
    typename TMap::iterator it;
    bool inserted;
    
    
    TMap map;
    map.burst_threshold(8);
    
    for(size_t i = 0; i < nb_values; i++) {
        std::tie(it, inserted) = map.insert(utils::get_key<char_tt>(i), utils::get_value<value_tt>(i));
        
        BOOST_CHECK_EQUAL(it.key(), (utils::get_key<char_tt>(i)));
        BOOST_CHECK_EQUAL(*it, utils::get_value<value_tt>(i));
        BOOST_CHECK(inserted);
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values);
    
    for(size_t i = 0; i < nb_values; i++) {
        std::tie(it, inserted) = map.insert(utils::get_key<char_tt>(i), utils::get_value<value_tt>(i+1));
        
        BOOST_CHECK_EQUAL(it.key(), (utils::get_key<char_tt>(i)));
        BOOST_CHECK_EQUAL(*it, utils::get_value<value_tt>(i));
        BOOST_CHECK(!inserted);
    }
    
    for(size_t i = 0; i < nb_values; i++) {
        it = map.find(utils::get_key<char_tt>(i));
        
        BOOST_CHECK(it != map.end());
        BOOST_CHECK_EQUAL(it.key(), (utils::get_key<char_tt>(i)));
        BOOST_CHECK_EQUAL(*it, utils::get_value<value_tt>(i));
    }
    
    for(auto it = map.begin(); it != map.end(); ++it) {
        auto it_find = map.find(it.key());
        
        BOOST_CHECK(it_find != map.end());
        BOOST_CHECK_EQUAL(it_find.key(), it.key());
    }
}

BOOST_AUTO_TEST_CASE(test_insert_with_too_long_string) {
    tsl::htrie_map<char, int64_t, tsl::str_hash<char>, std::uint8_t> map;
    map.burst_threshold(8);
    
    for(std::size_t i=0; i < 1000; i++) {
        map.insert(utils::get_key<char>(i), utils::get_value<int64_t>(i));
    }
    
    const std::string long_string("a", map.max_key_size());
    BOOST_CHECK(map.insert(long_string, utils::get_value<int64_t>(1000)).second);
    
    const std::string too_long_string("a", map.max_key_size() + 1);
    BOOST_CHECK_THROW(map.insert(too_long_string, utils::get_value<int64_t>(1001)), std::length_error);
}


/**
 * erase
 */
BOOST_AUTO_TEST_CASE_TEMPLATE(test_erase_all, TMap, test_types) {
    // insert x values, delete all
    const size_t nb_values = 1000;
    TMap map = utils::get_filled_map<TMap>(nb_values, 8);
    
    auto it = map.erase(map.begin(), map.end());
    BOOST_CHECK(it == map.end());
    BOOST_CHECK(map.begin() == map.end());
    BOOST_CHECK(map.empty());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_erase_loop, TMap, test_types) {
    // insert x values, delete all one by one
    size_t nb_values = 1000;
    TMap map = utils::get_filled_map<TMap>(nb_values, 8);
    TMap map2 = utils::get_filled_map<TMap>(nb_values, 8);
    
    auto it = map.begin();
    // Use second map to check for key after delete as we may not copy the key with move-only types.
    auto it2 = map2.begin();
    while(it != map.end()) {
        it = map.erase(it);
        --nb_values;
        
        BOOST_CHECK_EQUAL(map.count(it2.key()), 0);
        BOOST_CHECK_EQUAL(map.size(), nb_values);
        ++it2;
    }
    
    BOOST_CHECK(map.empty());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_erase_unknown, TMap, test_types) {
    using char_tt = typename TMap::char_type; 
    
    size_t nb_values = 1000;
    TMap map = utils::get_filled_map<TMap>(nb_values, 9);
    
    BOOST_CHECK_EQUAL(map.erase(utils::get_key<char_tt>(1001)), 0);
    BOOST_CHECK(map.erase(map.cbegin(), map.cbegin()) == map.begin());
    BOOST_CHECK(map == utils::get_filled_map<TMap>(nb_values, 8));
}


BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert_erase_insert, TMap, test_types) {
    // insert x/2 values, delete x/4 values, insert x/2 values, find each value
    using char_tt = typename TMap::char_type; 
    using value_tt = typename TMap::mapped_type;
    
    const size_t nb_values = 1000;
    typename TMap::iterator it;
    bool inserted;
    
    TMap map;
    map.burst_threshold(8);
    
    for(size_t i = 0; i < nb_values/2; i++) {
        std::tie(it, inserted) = map.insert(utils::get_key<char_tt>(i), utils::get_value<value_tt>(i));
        
        BOOST_CHECK_EQUAL(it.key(), (utils::get_key<char_tt>(i)));
        BOOST_CHECK_EQUAL(*it, utils::get_value<value_tt>(i));
        BOOST_CHECK(inserted);
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values/2);
    
    
    // Delete half
    for(size_t i = 0; i < nb_values/2; i++) {
        if(i%2 == 0) {
            BOOST_CHECK_EQUAL(map.erase(utils::get_key<char_tt>(i)), 1);
            BOOST_CHECK(map.find(utils::get_key<char_tt>(i)) == map.end());
        }
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values/4);
    
    
    for(size_t i = nb_values/2; i < nb_values; i++) {
        std::tie(it, inserted) = map.insert(utils::get_key<char_tt>(i), utils::get_value<value_tt>(i));
        
        BOOST_CHECK_EQUAL(it.key(), (utils::get_key<char_tt>(i)));
        BOOST_CHECK_EQUAL(*it, utils::get_value<value_tt>(i));
        BOOST_CHECK(inserted);
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values-nb_values/4);
    
    for(size_t i = 0; i < nb_values; i++) {
        it = map.find(utils::get_key<char_tt>(i));
            
        if(i%2 == 0 && i < nb_values/2) {
            BOOST_CHECK(it == map.cend());
        }
        else {
            BOOST_CHECK_EQUAL(it.key(), (utils::get_key<char_tt>(i)));
            BOOST_CHECK_EQUAL(*it, utils::get_value<value_tt>(i));
        }
    }
}

/**
 * emplace
 */
BOOST_AUTO_TEST_CASE(test_emplace) {
    tsl::htrie_map<char, move_only_test> map;
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
    tsl::htrie_map<char, int64_t> map;
    BOOST_CHECK(map.begin() == map.end());
    BOOST_CHECK(map.begin() == map.cend());
    BOOST_CHECK(map.cbegin() == map.cend());
}

/**
 * operator== and operator!=
 */
BOOST_AUTO_TEST_CASE(test_compare) {
    tsl::htrie_map<char, int64_t> map =  {{"test1", 10}, {"test2", 20}, {"test3", 30}};
    tsl::htrie_map<char, int64_t> map2 = {{"test3", 30}, {"test2", 20}, {"test1", 10}};
    tsl::htrie_map<char, int64_t> map3 = {{"test1", 10}, {"test2", 20}, {"test3", -1}};
    tsl::htrie_map<char, int64_t> map4 = {{"test3", 30}, {"test2", 20}};
    
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
    tsl::htrie_map<char, int64_t> map = {{"test1", 10}, {"test2", 20}};
    
    map.clear();
    BOOST_CHECK_EQUAL(map.size(), 0);
    BOOST_CHECK(map.begin() == map.end());
    BOOST_CHECK(map.cbegin() == map.cend());
}

/**
 * operator=
 */
BOOST_AUTO_TEST_CASE(test_assign_operator) {
    tsl::htrie_map<char, int64_t> map = {{"test1", 10}, {"test2", 20}};
    BOOST_CHECK_EQUAL(map.size(), 2);
    
    map = {{"test3", 30}};
    BOOST_CHECK_EQUAL(map.size(), 1);
    BOOST_CHECK_EQUAL(map.at("test3"), 30);
}


BOOST_AUTO_TEST_CASE(test_copy) {
    tsl::htrie_map<char, int64_t> map = utils::get_filled_map<tsl::htrie_map<char, int64_t>>(1000, 8);
    tsl::htrie_map<char, int64_t> map2 = map;
    tsl::htrie_map<char, int64_t> map3;
    map3 = map;
    
    BOOST_CHECK(map == map2);
    BOOST_CHECK(map == map3);
}


BOOST_AUTO_TEST_CASE(test_move) {
    const std::size_t nb_elements = 1000;
    const tsl::htrie_map<char, int64_t> init_map = utils::get_filled_map<tsl::htrie_map<char, int64_t>>(nb_elements, 8);
    
    tsl::htrie_map<char, int64_t> map = utils::get_filled_map<tsl::htrie_map<char, int64_t>>(nb_elements, 8);
    tsl::htrie_map<char, int64_t> map2 = std::move(map);
    
    BOOST_CHECK(map.empty());
    BOOST_CHECK(map.begin() == map.end());
    BOOST_CHECK_EQUAL(map2.size(), nb_elements);
    BOOST_CHECK(map2 == init_map);
    
    
    tsl::htrie_map<char, int64_t> map3;
    map3 = std::move(map2);
    
    BOOST_CHECK(map2.empty());
    BOOST_CHECK(map2.begin() == map2.end());
    BOOST_CHECK_EQUAL(map3.size(), nb_elements);
    BOOST_CHECK(map3 == init_map);
    
    map2 = {{"test1", 10}};
    BOOST_CHECK(map2 == (tsl::htrie_map<char, int64_t>{{"test1", 10}}));
}

/**
 * at
 */
BOOST_AUTO_TEST_CASE(test_at) {
    // insert x values, use at for known and unknown values.
    tsl::htrie_map<char, int64_t> map = {{"test1", 10}, {"test2", 20}};
    map.insert("test4", 40);
    
    BOOST_CHECK_EQUAL(map.at("test1"), 10);
    BOOST_CHECK_EQUAL(map.at("test2"), 20);
    BOOST_CHECK_THROW(map.at("test3"), std::out_of_range);
    BOOST_CHECK_EQUAL(map.at("test4"), 40);
    
    
    const tsl::htrie_map<char, int64_t> map_const = {{"test1", 10}, {"test2", 20}, {"test4", 40}};
    
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
    tsl::htrie_map<char, int64_t> map = {{"test1", 10}, {"test2", 20}};
    
    BOOST_CHECK_EQUAL(map["test1"], 10);
    BOOST_CHECK_EQUAL(map["test2"], 20);
    BOOST_CHECK_EQUAL(map["test3"], int64_t());
    
    map["test3"] = 30;
    BOOST_CHECK_EQUAL(map["test3"], 30);
    
    BOOST_CHECK_EQUAL(map.size(), 3);
}

/**
 * swap
 */
BOOST_AUTO_TEST_CASE(test_swap) {
    tsl::htrie_map<char, int64_t> map = {{"test1", 10}, {"test2", 20}};
    tsl::htrie_map<char, int64_t> map2 = {{"test3", 30}, {"test4", 40}, {"test5", 50}};
    
    using std::swap;
    swap(map, map2);
    
    BOOST_CHECK(map == (tsl::htrie_map<char, int64_t>{{"test3", 30}, {"test4", 40}, {"test5", 50}}));
    BOOST_CHECK(map2 == (tsl::htrie_map<char, int64_t>{{"test1", 10}, {"test2", 20}}));
}

