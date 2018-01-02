#ifndef TSL_UTILS_H
#define TSL_UTILS_H

#include <boost/numeric/conversion/cast.hpp>
#include <memory>
#include <ostream>
#include <string>


class move_only_test {
public:
    explicit move_only_test(int64_t value) : m_value(new int64_t(value)) {
    }
    
    move_only_test(const move_only_test&) = delete;
    move_only_test(move_only_test&&) = default;
    move_only_test& operator=(const move_only_test&) = delete;
    move_only_test& operator=(move_only_test&&) = default;
    
    friend std::ostream& operator<<(std::ostream& stream, const move_only_test& value) {
        if(value.m_value == nullptr) {
            stream << "null";
        }
        else {
            stream << *value.m_value;
        }
        
        return stream;
    }
    
    friend bool operator==(const move_only_test& lhs, const move_only_test& rhs) { 
        if(lhs.m_value == nullptr || rhs.m_value == nullptr) {
            return lhs.m_value == nullptr && rhs.m_value == nullptr;
        }
        else {
            return *lhs.m_value == *rhs.m_value; 
        }
    }
    
    friend bool operator!=(const move_only_test& lhs, const move_only_test& rhs) { 
        return !(lhs == rhs); 
    }
    
    friend bool operator<(const move_only_test& lhs, const move_only_test& rhs) {
        if(lhs.m_value == nullptr && rhs.m_value == nullptr) {
            return false;
        }
        else if(lhs.m_value == nullptr) {
            return true;
        }
        else if(rhs.m_value == nullptr) {
            return false;
        }
        else {
            return *lhs.m_value < *rhs.m_value; 
        }
    }
    
    int64_t value() const {
        return *m_value;
    }
private:    
    std::unique_ptr<int64_t> m_value;
};


class throw_move_test {
public:
    explicit throw_move_test(int64_t value) : m_value(value) {
    }
    
    throw_move_test(const throw_move_test&) = default;
    
    throw_move_test(throw_move_test&& other) noexcept(false): m_value(other.m_value) {
    }
    
    throw_move_test& operator=(const throw_move_test&) = default;
    
    throw_move_test& operator=(throw_move_test&& other) noexcept(false) {
        m_value = other.m_value;
        return *this;
    }
    
    
    operator int64_t() const {
        return m_value;
    }
private:    
    int64_t m_value;
};

class utils {
public:
    template<typename CharT>
    static std::basic_string<CharT> get_key(size_t counter);
    
    template<typename T>
    static T get_value(size_t counter);
    
    template<typename TMap>
    static TMap get_filled_map(size_t nb_elements, size_t burst_threshold);
};




template<>
inline std::basic_string<char> utils::get_key<char>(size_t counter) {
    return "Key " + std::to_string(counter);
}




template<>
inline int64_t utils::get_value<int64_t>(size_t counter) {
    return boost::numeric_cast<int64_t>(counter*2);
}

template<>
inline std::string utils::get_value<std::string>(size_t counter) {
    return "Value " + std::to_string(counter);
}

template<>
inline throw_move_test utils::get_value<throw_move_test>(size_t counter) {
    return throw_move_test(boost::numeric_cast<int64_t>(counter*2));
}

template<>
inline move_only_test utils::get_value<move_only_test>(size_t counter) {
    return move_only_test(boost::numeric_cast<int64_t>(counter*2));
}



template<typename TMap>
inline TMap utils::get_filled_map(size_t nb_elements, size_t burst_threshold) {
    using char_tt = typename TMap::char_type; 
    using value_tt = typename TMap::mapped_type;
    
    TMap map;
    map.burst_threshold(burst_threshold);
    for(size_t i = 0; i < nb_elements; i++) {
        map.insert(utils::get_key<char_tt>(i), utils::get_value<value_tt>(i));
    }
    
    return map;
}

#endif
