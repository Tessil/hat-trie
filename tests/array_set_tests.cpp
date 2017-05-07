#define BOOST_TEST_DYN_LINK

#include <tuple>
#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>

#include "array_set.h"
#include "utils.h"


using test_types = boost::mpl::list<
                        tsl::array_set<char>,
                        tsl::array_set<wchar_t>,
                        tsl::array_set<char16_t>,
                        tsl::array_set<char32_t>,
                        tsl::array_set<char, tsl::str_hash<char>, tsl::str_equal<char>, false>,
                        tsl::array_set<wchar_t, tsl::str_hash<wchar_t>, tsl::str_equal<wchar_t>, false>,
                        tsl::array_set<char16_t, tsl::str_hash<char16_t>, tsl::str_equal<char16_t>, false>,
                        tsl::array_set<char32_t, tsl::str_hash<char32_t>, tsl::str_equal<char32_t>, false>
                        >;


/**
 * insert
 */                                      
BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert, ASet, test_types) {
    // insert x values, insert them again, check values
    using char_tt = typename ASet::char_type;
    
    
    const size_t nb_values = 1000;
    ASet map;
    typename ASet::iterator it;
    bool inserted;
    
    for(size_t i = 0; i < nb_values; i++) {
        const auto key = utils::get_key<char_tt>(i);
        std::tie(it, inserted) = map.insert(key);
        
        BOOST_CHECK(map.key_eq()(it.key(), it.key_size(), key.c_str(), key.size()));
        BOOST_CHECK(inserted);
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values);
    
    for(size_t i = 0; i < nb_values; i++) {
        const auto key = utils::get_key<char_tt>(i);
        std::tie(it, inserted) = map.insert(key);
        
        BOOST_CHECK(map.key_eq()(it.key(), it.key_size(), key.c_str(), key.size()));
        BOOST_CHECK(!inserted);
    }
    
    for(size_t i = 0; i < nb_values; i++) {
        const auto key = utils::get_key<char_tt>(i);
        it = map.find(key);
        
        BOOST_CHECK(it != map.end());
        BOOST_CHECK(map.key_eq()(it.key(), it.key_size(), key.c_str(), key.size()));
    }
}
