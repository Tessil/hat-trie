[![Build Status](https://travis-ci.org/Tessil/array-hash.svg?branch=master)](https://travis-ci.org/Tessil/array-hash) [![Build status](https://ci.appveyor.com/api/projects/status/t50rr5bm6ejf350x/branch/master?svg=true)](https://ci.appveyor.com/project/Tessil/array-hash/branch/master)
## A C++ implementation of a fast and memory efficient hash map/set for strings 

Cache conscious hash map and hash set for strings based on the "Cache-conscious collision resolution in string hash tables." (Askitis Nikolas and Justin Zobel, 2005) paper.

Thanks to its cache friendliness, the structure provides fast lookups while keeping a low memory usage. The main drawback is the rehash process which is a bit slow and need some spare memory to copy the strings from the old hash table to the new hash table (it can’t use `std::move` as the other hash tables using `std::string` as key).

<p align="center">
  <img src="https://tessil.github.io/images/array_hash.png" width="500px" />
</p>

Four classes are provided: `tsl::array_map`, `tsl::array_set`, `tsl::array_pg_map` and `tsl::array_pg_set`. The first two are faster and use a power of two growth policy, the last two use a prime growth policy instead and are able to cope better with a poor hash function. Use the prime version if there is a chance of repeating patterns in the lower bits of your hash (e.g. you are storing pointers with an identity hash function). See [GrowthPolicy](https://github.com/Tessil/array-hash#growth-policy) for details.

A **benchmark** of `tsl::array_map` against other hash maps can be found [here](https://tessil.github.io/2016/08/29/benchmark-hopscotch-map.html). This page also gives some advices on which hash table structure you should try for your use case (useful if you are a bit lost with the multiple hash tables implementations in the `tsl` namespace). You can also find another benchmark on the [`tsl::hat-trie`](https://github.com/Tessil/hat-trie#benchmark) page.

### Overview
- Header-only library, just add the project to your include path and you are ready to go.
- Low memory usage with good performances, see the [benchmark](https://tessil.github.io/2016/08/29/benchmark-hopscotch-map.html) for some numbers.
- Support for move-only and non-default constructible values.
- Strings with null characters inside them are supported (you can thus store binary data as key).
- If the hash is known before a lookup, it is possible to pass it as parameter to speed-up the lookup.
- By default the maximum allowed size for a key is set to 65 535. This can be raised through the `KeySizeT` template parameter.
- By default the maximum size of the map is limited to 4 294 967 296 elements. This can be raised through the `IndexSizeT` template parameter.

### Differences compare to `std::unordered_map`
`tsl::array_map` tries to have an interface similar to `std::unordered_map`, but some differences exist:
- Iterator invalidation doesn't behave in the same way, any operation modifying the hash table invalidate them (see [API](https://tessil.github.io/array-hash/doc/html/classtsl_1_1array__map.html#details) for details).
- References and pointers to keys or values in the map are invalidated in the same way as iterators to these keys-values.
- Erase operations have an amortized runtime complexity of O(1) for `tsl::array_map`. An erase operation will delete the key immediatly but for the value part of the map, the deletion may be delayed. The destructor of the value is only called when the ratio between the size of the map and the size of the map + the number of deleted values still stored is low enough. The method `shrink_to_fit` may be called to force the deletion.
- The key and the value are stored separatly and not in a `std::pair<const Key, T>`. Methods like `insert` or `emplace` take the key and the value separatly instead of a `std::pair`. The insert method looks like `std::pair<iterator, bool> insert(const CharT* key, const T& value)` instead of `std::pair<iterator, bool> insert(const std::pair<const Key, T>& value)` (see [API](https://tessil.github.io/array-hash/doc/html/classtsl_1_1array__map.html) for details).
- For iterators, `operator*()` and `operator->()` return a reference and a pointer to the value `T` instead of `std::pair<const Key, T>`. For an access to the key string, the `key()` (which returns a `const CharT*`) or `key_sv()` (which returns a `std::basic_string_view<CharT>`) method of the iterator must be called.
- No support for some bucket related methods (like bucket_size, bucket, ...).


These differences also apply between `std::unordered_set` and `tsl::array_set`.

Thread-safety and exception guarantees are similar to the STL containers.

### Hash function

To avoid dependencies, the default hash function is a simple [FNV-1a](https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function#FNV-1a_hash) hash function. If you can, I recommend to use something like [CityHash](https://github.com/google/cityhash), MurmurHash, [FarmHash](https://github.com/google/farmhash), ... for better performances. On the tests I did, CityHash64 offers a ~40% improvement on reads compared to FNV-1a.


```c++
#include <city.h>

struct str_hash {
    std::size_t operator()(const char* key, std::size_t key_size) const {
        return CityHash64(key, key_size);
    }
};

tsl::array_map<char, int, str_hash> map;
```

If you have access to `std::string_view` and you want to use the compiler provided hash implementation for strings.

```c++
#include <string_view>

struct str_hash {
    std::size_t operator()(const char* key, std::size_t key_size) const {
        return std::hash<std::string_view>()(std::string_view(key, key_size));
    }
};

tsl::array_map<char, int, str_hash> map;
```

The `std::hash<std::string>` can't be used efficiently as the structure doesn't store any `std::string` object. Any time a hash would be needed, a temporary `std::string` would have to be created.


### Growth policy

The library supports multiple growth policies through the `GrowthPolicy` template parameter. Three policies are provided by the library but you can easly implement your own if needed.

* **[tsl::power_of_two_growth_policy_ah.](https://tessil.github.io/array-hash/doc/html/classtsl_1_1power__of__two__growth__policy__ah.html)** Default policy used by `tsl::array_map/set`. This policy keeps the size of the bucket array of the hash table to a power of two. This constraint allows the policy to avoid the usage of the slow modulo operation to map a hash to a bucket, instead of <code>hash % 2<sup>n</sup></code>, it uses <code>hash & (2<sup>n</sup> - 1)</code> (see [fast modulo](https://en.wikipedia.org/wiki/Modulo_operation#Performance_issues)). Fast but this may cause a lot of collisions with a poor hash function as the modulo with a power of two only masks the most significant bits in the end.
* **[tsl::prime_growth_policy_ah.](https://tessil.github.io/array-hash/doc/html/classtsl_1_1prime__growth__policy__ah.html)** Default policy used by `tsl::array_pg_map/set`. The policy keeps the size of the bucket array of the hash table to a prime number. When mapping a hash to a bucket, using a prime number as modulo will result in a better distribution of the hash across the buckets even with a poor hash function. To allow the compiler to optimize the modulo operation, the policy use a lookup table with constant primes modulos (see [API](https://tessil.github.io/array-hash/doc/html/classtsl_1_1prime__growth__policy__ah.html#details) for details). Slower than `tsl::power_of_two_growth_policy_ah` but more secure.
* **[tsl::mod_growth_policy_ah.](https://tessil.github.io/array-hash/doc/html/classtsl_1_1mod__growth__policy__ah.html)** The policy grows the map by a customizable growth factor passed in parameter. It then just use the modulo operator to map a hash to a bucket. Slower but more flexible.


To implement your own policy, you have to implement the following interface.

```c++
struct custom_policy {
    // Called on hash table construction, min_bucket_count_in_out is the minimum size
    // that the hash table needs. The policy can change it to a higher bucket count if needed
    custom_policy(std::size_t& min_bucket_count_in_out);
    
    // Return the bucket for the corresponding hash
    std::size_t bucket_for_hash(std::size_t hash) const noexcept;
    
    // Return the number of buckets that should be used on next growth
    std::size_t next_bucket_count() const;
    
    // Maximum number of buckets supported by the policy
    std::size_t max_bucket_count() const;
}
```

### Installation
To use array-hash, just add the project to your include path. It is a **header-only** library.

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

The API can be found [here](https://tessil.github.io/array-hash/doc_without_string_view/html). If `std::string_view` is available, the API changes slightly and can be found [here](https://tessil.github.io/array-hash/doc/html/).


### Example
```c++
#include <iostream>
#include <tsl/array_map.h>
#include <tsl/array_set.h>


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
