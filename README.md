**Warning: the library is still in beta stage, everything should work fine but some features are still missing (i.e. prefix search) while some others could be optimized.**

## A C++ implementation of a fast and memory efficient HAT-trie

Trie implementation based on the "HAT-trie: A Cache-conscious Trie-based Data Structure for Strings." (Askitis Nikolas and  Sinha Ranjan, 2007) paper.

The library provides two classes: `tsl::htrie_map` and `tsl::htrie_set`.

### Installation
To use the hat-trie library, just add the [src/](src/) directory to your include path. It's a **header-only** library. Don't forget the sub-module.

The code should work with any C++11 standard-compliant compiler and has been tested with GCC 4.8.4, Clang 3.5.0 and Visual Studio 2015.

To run the tests you will need the Boost Test library and CMake. 

```bash
git clone --recursive https://github.com/Tessil/hat-trie.git
cd hat-trie
mkdir build
cd build
cmake ..
make
./test_hat_trie
```

### Benchmark

The benchmark consists in inserting all the titles in the main namespace of the wikipedia archive, check the size of the structure and read all the titles again.

* Dataset: enwiki-20170320-all-titles-in-ns0.gz
* Size: 262.7 MiB
* Number of keys: 13 099 148
* Avg key length: 19
* Max key length: 251

Each title is associated with an int (32 bits). All the hash based structures use [CityHash64](https://github.com/google/cityhash) as hash function.

The benchmark was compiled with GCC 6.3 and ran on Debian Stretch x64 with an Intel i5-5200u and 8Go of RAM. Best of 20 runs was taken.

The code of the benchmark can be found on [Gist](https://gist.github.com/Tessil/2dacd14d46b35287acb81dd149276dec).

| Library | Data structure | Space (MiB) | Insert (ns/key) | Read (ns/key) |
|---------|----------------|-------:|--------:|-----:|
| [tsl::htrie_map](https://github.com/Tessil/hat-trie) | HAT-trie | 442.21 | 249.48 | 57.41 |
| [tsl::htrie_map](https://github.com/Tessil/hat-trie) <br/> max_load_factor=2 | HAT-trie | 566.73 | 261.09 | **49.29** |
| [tsl::htrie_map](https://github.com/Tessil/hat-trie) <br/> max_load_factor=10 | HAT-trie | **397.35** | 285.41 | 66.32 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) | Double-array trie  | 1254.41 | 282.54 | 54.58 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) ORDERED=false | Double-array trie  | 1254.47 | 266.73 | 55.88 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) | Double-array reduced trie  | 1167.78 | 256.43 | 69.09 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) ORDERED=false | Double-array reduced trie  | 1167.84 | 240.78 | 69.39 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) | Double-array prefix trie | 619.38 | 245.89 | 58.63 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) ORDERED=false | Double-array prefix trie  | 619.37 | **190.09** | 58.48 |
| [hat-trie](https://github.com/dcjones/hat-trie) (C) | HAT-trie | 518.52 | 501.86 | 86.72 |
| [JudySL](http://judy.sourceforge.net/) (C) | Judy array | 614.21 | 277.27 | 113.29 |
| [tsl::array_map](https://github.com/Tessil/array-hash) | Array hash | 555.35 | 247.96 | 128.79 |
| [spp::sparse_hash_map](https://github.com/greg7mdp/sparsepp) | Sparse hash table | 912.46 | 418.88 | 158.56 |

### Example
```c++
#include <iostream>
#include <string>
#include "htrie_map.h"
#include "htrie_set.h"


int main() {
    tsl::htrie_map<char, int> map = {{"one", 1}, {"two", 2}};
    map["three"] = 3;
    map["four"] = 4;
    
    map.insert("five", 5);
    map.insert_ks("six_with_extra_chars_we_ignore", 3, 6);
    
    map.erase("two");
    
    /*
     * Due to the compression on the common prefixes, the letters of the string 
     * are not always stored contiguously. When we retrieve the key, we have to 
     * construct it.
     * 
     * To avoid a heap-allocation at each iteration (when SSO doesn't occur), 
     * we can reuse the key_buffer to construct the key.
     */
    std::string key_buffer;
    for(auto it = map.begin(); it != map.end(); ++it) {
        it.key(key_buffer);
        std::cout << "{" << key_buffer << ", " << it.value() << "}" << std::endl;
    }
    
    /*
     * If you don't care about the allocation.
     */
    for(auto it = map.begin(); it != map.end(); ++it) {
        std::cout << "{" << it.key() << ", " << *it << "}" << std::endl;
    }
    
    
    
    tsl::htrie_set<char> set = {"one", "two", "three"};
    set.insert({"four", "five"});
    
    for(auto it = set.begin(); it != set.end(); ++it) {
        it.key(key_buffer);
        std::cout << "{" << key_buffer << "}" << std::endl;
    }
} 
```


### License

The code is licensed under the MIT license, see the [LICENSE file](LICENSE) for details.
