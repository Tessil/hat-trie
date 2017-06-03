[![Build Status](https://travis-ci.org/Tessil/hat-trie.svg?branch=master)](https://travis-ci.org/Tessil/hat-trie) [![Build status](https://ci.appveyor.com/api/projects/status/ieafyj08ewb7dfa7/branch/master?svg=true)](https://ci.appveyor.com/project/Tessil/hat-trie/branch/master)
## A C++ implementation of a fast and memory efficient HAT-trie

Trie implementation based on the "HAT-trie: A Cache-conscious Trie-based Data Structure for Strings." (Askitis Nikolas and  Sinha Ranjan, 2007) paper. For now, only the pure HAT-trie has been implemented, the hybrid version may arrive later.

The library provides an efficient and compact way to store a set or a map of strings by compressing the common prefixes. It also allows to search for keys that match a prefix.

It's a well adapted structure to store a large number of strings.

<p align="center">
  <img src="https://tessil.github.io/images/hat-trie.png" width="600px" />
</p>

For the àrray hash part, the [array-hash](https://github.com/Tessil/array-hash) project is used and included as git subtree.

The library provides two classes: `tsl::htrie_map` and `tsl::htrie_set`.

### Overview

- Header-only library, just include [src/](src/) to your include path and you're ready to go.
- Low memory usage while keeping reasonable performances (see [benchmark](https://github.com/Tessil/hat-trie#benchmark)).
- Allow prefix searches through `equal_prefix_range` (usefull for autocompletion for example).
- Keys are not ordered as they are partially stored in a hash map.
- All operations modifying the data structure (insert, emplace, erase, ...) invalidate the iterators. 
- Support for any type of value as long at it's either copy-constructible or both nothrow move constructible and nothrow move assignable.
- The balance between speed and memory usage can be modified through `max_load_factor`. A lower max load factor will increase the speed, a higher one will reduce the memory usage. Its default value is set to 8.0.
- The default burst threshold, which is the maximum size of an array hash node before a burst occurs, is set to 16 384 which provides good performances for exact searches. If you mainly use prefix searches, you may want to reduce it to something like 8 192 or 4 096 for faster iteration on the results through `burst_threshold`.
- By default the maximum allowed size for a key is set to 65 535. This can be raised through the `KeySizeT` template parameter.

Thread-safety and exception guarantees are similar to the STL containers.

### Hash function

To avoid dependencies, the default hash function is a simple [FNV-1a](https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function#FNV-1a_hash) hash function. If you can, I recommend to use something like [CityHash](https://github.com/google/cityhash), MurmurHash, [FarmHash](https://github.com/google/farmhash), ... for better performances. On the tests I did, CityHash64 offers a ~20% improvement on reads compared to FNV-1a.


```c++
#include <city.h>

struct str_hash {
    std::size_t operator()(const char* key, std::size_t key_size) const {
        return CityHash64(key, key_size);
    }
};

tsl::htrie_map<char, int, str_hash> map;
```

If you have access to `std::string_view` and you want to use the compiler provided hash implementation for strings.

```c++
#include <string_view>

struct str_hash {
    std::size_t operator()(const char* key, std::size_t key_size) const {
        return std::hash<std::string_view>()(std::string_view(key, key_size));
    }
};

tsl::htrie_map<char, int, str_hash> map;
```

The `std::hash<std::string>` can't be used efficiently as the structure doesn't store any `std::string` object. Any time a hash would be needed a temporary `std::string` would have to be created.

### Benchmark

#### Wikipedia dataset
The benchmark consists in inserting all the titles from the main namespace of the Wikipedia archive into the data structure, check the used memory space and search for all the titles again in the data structure.

* Dataset: [enwiki-20170320-all-titles-in-ns0.gz](https://dumps.wikimedia.org/enwiki/20170320/)
* Size: 262.7 MiB
* Number of keys: 13 099 148
* Average key length: 19.90
* Median key length: 17
* Max key length: 251

Each title is associated with an int (32 bits). All the hash based structures use [CityHash64](https://github.com/google/cityhash) as hash function and `reserve` is not called. 

Note that `tsl::hopscotch_map`, `std::unordered_map`, `google::dense_hash_map` and `spp::sparse_hash_map` use `std::string` as key which imposes a minimum size of 32 bytes (on x64) even if the key is only one character long. Other structures may be able to store one-character keys with 1 byte + 8 bytes for a pointer (on x64).

The benchmark was compiled with GCC 6.3 and ran on Debian Stretch x64 with an Intel i5-5200u and 8Go of RAM. Best of 20 runs was taken.

The code of the benchmark can be found on [Gist](https://gist.github.com/Tessil/72e11891fc155f5b2eb53de22cbc4053).

##### Unsorted

The *enwiki-20170320-all-titles-in-ns0.gz* dataset is alphabetically sorted. For this benchmark, we first shuffle the dataset through [shuf(1)](https://linux.die.net/man/1/shuf) to avoid a biased sorted dataset.

| Library | Data structure | Space (MiB) | Insert (ns/key) | Read (ns/key) |
|---------|----------------|-------:|--------:|-----:|
| [tsl::htrie_map](https://github.com/Tessil/hat-trie) | HAT-trie | **402.25** | 643.10 | 250.87 |
| [tsl::htrie_map](https://github.com/Tessil/hat-trie) <br/> max_load_factor=4 | HAT-trie | 468.50 | 638.66 | 212.90 |
| [tsl::htrie_map](https://github.com/Tessil/hat-trie) <br/> max_load_factor=2 | HAT-trie | 566.52 | 630.61 | 201.10 |
| [tsl::htrie_map](https://github.com/Tessil/hat-trie) <br/> max_load_factor=1 | HAT-trie | 709.81 | 645.76 | 190.87 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) | Double-array trie  | 1254.41 | 1102.93 | 557.20 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) ORDERED=false | Double-array trie  | 1254.41 | 1089.78 | 570.13 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) | Double-array reduced trie  | 1167.79 | 1076.68 | 645.79 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) ORDERED=false | Double-array reduced trie  | 1167.85 | 1065.43 | 641.98 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) | Double-array prefix trie | 496.54 | 1096.90 | 628.01 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) ORDERED=false | Double-array prefix trie  | 496.60 | 1048.40 | 628.94 |
| [hat-trie](https://github.com/dcjones/hat-trie)<sup>1</sup> (C) | HAT-trie | 501.50 | 917.49 | 261.00 |
| [JudySL](http://judy.sourceforge.net/) (C) | Judy array | 628.37 | 884.29 | 803.58 |
| [JudyHS](http://judy.sourceforge.net/) (C) | Judy array | 719.47 | 476.79 | 417.15 |
| [tsl::array_map](https://github.com/Tessil/array-hash) | Array hash table | 678.73 | 603.94 |  138.24 |
| [tsl::hopscotch_map](https://github.com/Tessil/hopscotch-map) | Hash table | 1077.99 | **368.26** |  **119.49** |
| [google::dense_hash_map](https://github.com/sparsehash/sparsehash) | Hash table | 1677.11 | 466.60 | 138.87 |
| [spp::sparse_hash_map](https://github.com/greg7mdp/sparsepp) | Sparse hash table | 917.10 | 769.00 | 175.59 |
| [std::unordered_map](http://en.cppreference.com/w/cpp/container/unordered_map) | Hash table | 1246.60 | 590.88 | 173.58  |

1. As the hash function can't be passed in parameter, the code of the library itself is modified to use CityHash64.

##### Sorted

The key are inserted and read in alphabetical order.

| Library | Data structure | Space (MiB) | Insert (ns/key) | Read (ns/key) |
|---------|----------------|-------:|--------:|-----:|
| [tsl::htrie_map](https://github.com/Tessil/hat-trie) | HAT-trie | **393.22** | 255.76 | 68.08 |
| [tsl::htrie_map](https://github.com/Tessil/hat-trie) <br/> max_load_factor=4 | HAT-trie | 461.80 | 248.88 | 59.23 |
| [tsl::htrie_map](https://github.com/Tessil/hat-trie) <br/> max_load_factor=2 | HAT-trie | 541.21 | 230.13 | 53.50 |
| [tsl::htrie_map](https://github.com/Tessil/hat-trie) <br/> max_load_factor=1 | HAT-trie | 689.70 | 243.84 | **49.22** |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) | Double-array trie  | 1254.41 | 278.51 | 54.72 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) ORDERED=false | Double-array trie  | 1254.41 | 264.43 | 56.02 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) | Double-array reduced trie  | 1167.78 | 254.60 | 69.18 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) ORDERED=false | Double-array reduced trie  | 1167.78 | 241.45 | 69.67 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) | Double-array prefix trie | 619.38 | 246.88 | 57.83 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) ORDERED=false | Double-array prefix trie | 619.38 | **187.98** | 58.56 |
| [hat-trie](https://github.com/dcjones/hat-trie)<sup>2</sup> (C) | HAT-trie | 518.52 | 503.01 | 86.40 |
| [JudySL](http://judy.sourceforge.net/) (C) | Judy array | 614.27 | 279.07 | 113.47 |
| [JudyHS](http://judy.sourceforge.net/) (C) | Judy array | 719.47 | 439.66 | 372.25 |
| [tsl::array_map](https://github.com/Tessil/array-hash) | Array hash table | 682.99 | 612.31 | 139.16  |
| [tsl::hopscotch_map](https://github.com/Tessil/hopscotch-map) | Hash table | 1078.02 | 375.19 | 118.08 |
| [google::dense_hash_map](https://github.com/sparsehash/sparsehash) | Hash table | 1683.07 | 483.95 | 137.09 |
| [spp::sparse_hash_map](https://github.com/greg7mdp/sparsepp) | Sparse hash table | 918.70 | 772.03 | 176.64 |
| [std::unordered_map](http://en.cppreference.com/w/cpp/container/unordered_map) | Hash table | 1246.65 | 594.85 | 173.54 |

2. As the hash function can't be passed in parameter, the code of the library itself is modified to use CityHash64.


#### Dr. Askitis dataset

The benchmark consists in inserting all the words from the "Distinct Strings" dataset of Dr. Askitis into the data structure, check the used memory space and search for all the words from the "Skew String Set 1" dataset in the data structure.

* Dataset: [distinct_1](http://web.archive.org/web/20120206015921/http://www.naskitis.com/) (write) / [skew1_1](http://web.archive.org/web/20120206015921/http://www.naskitis.com/) (read)
* Size: 290.45 MiB / 1 029.46 MiB
* Number of keys: 28 772 169 / 177 999 203
* Average key length: 9.59 / 5.06
* Median key length: 8 / 4
* Max key length: 126 / 62

The benchmark protocol is the same as for the [Wikipedia dataset](https://github.com/Tessil/hat-trie#wikipedia-dataset).

The test is similar to the one on the [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) homepage but it corrects some of its short-comings. The cedar's benchmark creates a `std::string` on each search for the structures using  `std::string` as key putting them in a huge disadvantage compared to the structures taking a `const char *` in parameter. It also leaves the overhead of I/O operations into the benchmark (in our case all the strings of the file are first loaded in memory before the benchmark). And it uses the default hash function of each hash based data structure (while we use CityHash64 for everyone for a fair comparison).


| Library | Data structure | Space (MiB) | Insert (ns/key) | Read (ns/key) |
|---------|----------------|-------:|--------:|-----:|
| [tsl::htrie_map](https://github.com/Tessil/hat-trie) | HAT-trie | **601.79** | 485.45 | 77.80 |
| [tsl::htrie_map](https://github.com/Tessil/hat-trie) <br/> max_load_factor=4 | HAT-trie | 764.98 | 491.78 | 75.48 |
| [tsl::htrie_map](https://github.com/Tessil/hat-trie) <br/> max_load_factor=2 | HAT-trie | 999.34 | 496.78 | 72.53 |
| [tsl::htrie_map](https://github.com/Tessil/hat-trie) <br/> max_load_factor=1 | HAT-trie | 1341.97 | 520.66 | 72.45 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) | Double-array trie | 1100.05 | 682.25 | 71.98 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) ORDERED=false | Double-array trie | 1100.05 | 668.75 | 71.95 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) | Double-array reduced trie | 926.04 | 684.38 | 79.11 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) ORDERED=false | Double-array reduced trie | 925.98 | 672.14 | 79.02 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) | Double-array prefix trie | 712.59 | 831.71 | 75.83 |
| [cedar](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) ORDERED=false | Double-array prefix trie | 712.31 | 786.93 | 75.89 |
| [hat-trie](https://github.com/dcjones/hat-trie)<sup>3</sup> (C) | HAT-trie | 784.32 | 743.34 | 93.58 |
| [JudySL](http://judy.sourceforge.net/) (C) | Judy array | 1023.11 | 535.02 | 202.36 |
| [JudyHS](http://judy.sourceforge.net/) (C) | Judy array | 999.97 | 456.09 | 148.36 |
| [tsl::array_map](https://github.com/Tessil/array-hash) | Array hash table | 1031.67 | 545.82 | 46.41 |
| [tsl::hopscotch_map](https://github.com/Tessil/hopscotch-map) | Hash table | 1611.54 | **288.70** | 47.05 |
| [google::dense_hash_map](https://github.com/sparsehash/sparsehash) | Hash table | 2636.31 | 317.66 | **43.62** |
| [spp::sparse_hash_map](https://github.com/greg7mdp/sparsepp) | Sparse hash table | 1417.61 | 586.26 | 56.00 |
| [std::unordered_map](http://en.cppreference.com/w/cpp/container/unordered_map) | Hash table | 2110.19 | 554.02 | 105.05 |

3. As the hash function can't be passed in parameter, the code of the library itself is modified to use CityHash64.

### Installation
To use the hat-trie library, just add the [src/](src/) directory to your include path. It's a **header-only** library. 

The code should work with any C++11 standard-compliant compiler and has been tested with GCC 4.8.4, Clang 3.5.0 and Visual Studio 2015.

To run the tests you will need the Boost Test library and CMake. 

```bash
git clone https://github.com/Tessil/hat-trie.git
cd hat-trie
mkdir build
cd build
cmake ..
make
./test_hat_trie
```

### Usage

The API can be found [here](https://tessil.github.io/hat-trie/doc_without_string_view/html). If `std::string_view` is available, the API changes slightly and can be found [here](https://tessil.github.io/hat-trie/doc/html/).

### Example
```c++
#include <iostream>
#include <string>
#include "htrie_map.h"
#include "htrie_set.h"


int main() {
    /*
     * Map of strings to int having char as character type. 
     * There is no support for wchar_t, char16_t or char32_t yet, 
     * but UTF-8 strings will work fine.
     */
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
     * we reuse the key_buffer to construct the key.
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
    
    
    tsl::htrie_map<char, int> map2 = {{"apple", 1}, {"mango", 2}, {"apricot", 3},
                                      {"mandarin", 4}, {"melon", 5}, {"macadamia", 6}};
    
    // Prefix search
    auto prefix_range = map2.equal_prefix_range("ma");
    
    // {mandarin, 4} {mango, 2} {macadamia, 6}
    for(auto it = prefix_range.first; it != prefix_range.second; ++it) {
        std::cout << "{" << it.key() << ", " << *it << "}" << std::endl;
    }
    
    
    
    
    tsl::htrie_set<char> set = {"one", "two", "three"};
    set.insert({"four", "five"});
    
    // {one} {two} {five} {four} {three}
    for(auto it = set.begin(); it != set.end(); ++it) {
        it.key(key_buffer);
        std::cout << "{" << key_buffer << "}" << std::endl;
    }
} 
```


### License

The code is licensed under the MIT license, see the [LICENSE file](LICENSE) for details.
