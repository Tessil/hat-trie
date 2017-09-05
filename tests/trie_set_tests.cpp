#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>
#include <tuple>
#include <utility>

#include "utils.h"
#include "tsl/htrie_set.h"


using test_types = boost::mpl::list<
                        tsl::htrie_set<char>
                        >;
                                    
                              
                                        
/**
 * insert
 */                                        
BOOST_AUTO_TEST_CASE_TEMPLATE(stest_insert, TMap, test_types) {
    // insert x values, insert them again, check values
    using char_tt = typename TMap::char_type; 
    
    const size_t nb_values = 100000;
    TMap set;
    typename TMap::iterator it;
    bool inserted;
    
    for(size_t i = 0; i < nb_values; i++) {
        std::tie(it, inserted) = set.insert(utils::get_key<char_tt>(i));
        
        BOOST_CHECK_EQUAL(it.key(), (utils::get_key<char_tt>(i)));
        BOOST_CHECK(inserted);
    }
    BOOST_CHECK_EQUAL(set.size(), nb_values);
    
    for(size_t i = 0; i < nb_values; i++) {
        std::tie(it, inserted) = set.insert(utils::get_key<char_tt>(i));
        
        BOOST_CHECK_EQUAL(it.key(), (utils::get_key<char_tt>(i)));
        BOOST_CHECK(!inserted);
    }
    
    for(size_t i = 0; i < nb_values; i++) {
        it = set.find(utils::get_key<char_tt>(i));
        
        BOOST_CHECK(it != set.end());
        BOOST_CHECK_EQUAL(it.key(), (utils::get_key<char_tt>(i)));
    }
    
    for(auto it = set.begin(); it != set.end(); ++it) {
        auto it_find = set.find(it.key());
        
        BOOST_CHECK(it_find != set.end());
        BOOST_CHECK_EQUAL(it_find.key(), it.key());
    }
}



/**
 * operator=
 */
BOOST_AUTO_TEST_CASE(stest_assign_operator) {
    tsl::htrie_set<char> set = {"test1", "test2"};
    BOOST_CHECK_EQUAL(set.size(), 2);
    
    set = {"test3"};
    BOOST_CHECK_EQUAL(set.size(), 1);
    BOOST_CHECK_EQUAL(set.count("test3"), 1);
}


BOOST_AUTO_TEST_CASE(stest_copy) {
    tsl::htrie_set<char> set = {"test1", "test2", "test3", "test4"};
    tsl::htrie_set<char> set2 = set;
    tsl::htrie_set<char> set3;
    set3 = set;
    
    BOOST_CHECK(set == set2);
    BOOST_CHECK(set == set3);
}

BOOST_AUTO_TEST_CASE(stest_move) {
    tsl::htrie_set<char> set = {"test1", "test2"};
    tsl::htrie_set<char> set2 = std::move(set);
    
    BOOST_CHECK(set.empty());
    BOOST_CHECK(set.begin() == set.end());
    BOOST_CHECK_EQUAL(set2.size(), 2);
    BOOST_CHECK(set2 == (tsl::htrie_set<char>{"test1", "test2"}));
    
    
    tsl::htrie_set<char> set3;
    set3 = std::move(set2);
    
    BOOST_CHECK(set2.empty());
    BOOST_CHECK(set2.begin() == set2.end());
    BOOST_CHECK_EQUAL(set3.size(), 2);
    BOOST_CHECK(set3 == (tsl::htrie_set<char>{"test1", "test2"}));
    
    set2 = {"test1"};
    BOOST_CHECK(set2 == (tsl::htrie_set<char>{"test1"}));
}
