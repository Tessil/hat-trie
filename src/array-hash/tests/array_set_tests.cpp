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
                        tsl::array_set<char, tsl::str_hash<char>, std::char_traits<char>, false>,
                        tsl::array_set<wchar_t, tsl::str_hash<wchar_t>, std::char_traits<wchar_t>, false>,
                        tsl::array_set<char16_t, tsl::str_hash<char16_t>, std::char_traits<char16_t>, false>,
                        tsl::array_set<char32_t, tsl::str_hash<char32_t>, std::char_traits<char32_t>, false>
                        >;


/**
 * insert
 */                                      
BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert, TMap, test_types) {
    // insert x values, insert them again, check values
    using char_tt = typename TMap::char_type; 
    using traits_tt = typename TMap:: traits_type; 
    
    
    const size_t nb_values = 1000;
    TMap map;
    typename TMap::iterator it;
    bool inserted;
    
    for(size_t i = 0; i < nb_values; i++) {
        std::tie(it, inserted) = map.insert(utils::get_key<char_tt, traits_tt>(i));
        
        BOOST_CHECK_EQUAL(it.key_size(), (utils::get_key<char_tt, traits_tt>(i)).size());
        BOOST_CHECK(traits_tt::compare(it.key(), utils::get_key<char_tt, traits_tt>(i).c_str(), it.key_size()) == 0);
        BOOST_CHECK(inserted);
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values);
    
    for(size_t i = 0; i < nb_values; i++) {
        std::tie(it, inserted) = map.insert(utils::get_key<char_tt, traits_tt>(i));
        
        BOOST_CHECK_EQUAL(it.key_size(), (utils::get_key<char_tt, traits_tt>(i)).size());
        BOOST_CHECK(traits_tt::compare(it.key(), utils::get_key<char_tt, traits_tt>(i).c_str(), it.key_size()) == 0);
        BOOST_CHECK(!inserted);
    }
    
    for(size_t i = 0; i < nb_values; i++) {
        it = map.find(utils::get_key<char_tt, traits_tt>(i));
        
        BOOST_CHECK(it != map.end());
        BOOST_CHECK_EQUAL(it.key_size(), (utils::get_key<char_tt, traits_tt>(i)).size());
        BOOST_CHECK(traits_tt::compare(it.key(), utils::get_key<char_tt, traits_tt>(i).c_str(), it.key_size()) == 0);
    }
}
