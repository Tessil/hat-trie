cmake_minimum_required(VERSION 3.10)

project(tsl_hat_trie VERSION 0.7.1)

add_library(tsl_hat_trie INTERFACE)
# Use tsl::hat_trie as target, more consistent with other libraries conventions (Boost, Qt, ...)
add_library(tsl::hat_trie ALIAS tsl_hat_trie)

target_include_directories(tsl_hat_trie INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_sources(tsl_hat_trie INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl/array-hash/array_growth_policy.h"
                                      "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl/array-hash/array_hash.h"
                                      "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl/array-hash/array_map.h"
                                      "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl/array-hash/array_set.h"
                                      "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl/htrie_hash.h"
                                      "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl/htrie_map.h"
                                      "${CMAKE_CURRENT_SOURCE_DIR}/include/tsl/htrie_set.h")

target_compile_features(tsl_hat_trie INTERFACE cxx_std_11)
