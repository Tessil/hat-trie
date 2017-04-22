[![Build Status](https://travis-ci.org/Tessil/array-hash.svg?branch=master)](https://travis-ci.org/Tessil/array-hash) [![Build status](https://ci.appveyor.com/api/projects/status/t50rr5bm6ejf350x/branch/master?svg=true)](https://ci.appveyor.com/project/Tessil/array-hash/branch/master)
## A C++ implementation of a fast and memory efficient hash map/set for strings 

Cache consious hash map for string based on the "Cache-conscious collision resolution in string hash tables." (Askitis Nikolas and Justin Zobel, 2005) paper.

<p align="center">
  <img src="https://tessil.github.io/images/array_hash.png" width="500px" />
</p>

The library provides two classes: `tsl::array_map` and `tsl::array_set`.

### Installation
To use array-hash, just add the [src/](src/) directory to your include path. It's a **header-only** library.

The code should work with any C++11 standard-compliant compiler and has been tested with GCC 4.8.4, Clang 3.5.0 and Visual Studio 2015.

To run the tests you will need the Boost Test library and CMake. 

```bash
git clone https://github.com/Tessil/array-hash.git
cd array-hash
mkdir build
cd build
cmake ..
make
./test_array_hash
```


### Usage
The API can be found [here](https://tessil.github.io/hopscotch-map/doc/html/). 


### Example
```c++
#include <iostream>
#include "array_map.h"
#include "array_set.h"


int main() {
    // Map of const char* to int
    tsl::array_map<char, int> map = {{"one", 1}, {"two", 2}};
    map["three"] = 3;
    map["four"] = 4;
    
    map.insert("five", 5);
    map.insert_ks("six_with_extra_chars_we_ignore", 3, 6);
    
    map.erase("two");
    
    // When template parameter StoreNullTerminator is true (default) and there is no
    // null character in the strings.
    for(auto it = map.begin(); it != map.end(); ++it) {
        std::cout << "{" << it.key() << ", " << it.value() << "}" << std::endl;
    }
    
    // If StoreNullTerminator is false for space efficiency or you are storing null characters, 
    // you can access to the size of the key.
    for(auto it = map.begin(); it != map.end(); ++it) {
        (std::cout << "{").write(it.key(), it.key_size()) << ", " << it.value() << "}" << std::endl;
    }
    
    // Better, use key_sv() if you compiler provides access to std::string_view.
    for(auto it = map.begin(); it != map.end(); ++it) {
        std::cout << "{" << it.key_sv() << ", " << it.value() << "}" << std::endl;
    }
    
    // Or if you just want the values.
    for(int value: map) {
        std::cout << "{" << value << "}" << std::endl;
    }


    // Map of const char32_t* to int
    tsl::array_map<char32_t, int> map_char32 = {{U"one", 1}, {U"two", 2}};
    map_char32[U"three"] = 3;
    
    
    // Set of const char*
    tsl::array_set<char> set = {"one", "two", "three"};
    set.insert({"four", "five"});
    
    for(auto it = set.begin(); it != set.end(); ++it) {
        std::cout << "{" << it.key() << "}" << std::endl;
    }
}
```


### License

The code is licensed under the MIT license, see the [LICENSE file](LICENSE) for details.
