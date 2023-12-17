// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_GRAPHS_HASHMAP_HPP_
#define PANDO_LIB_GALOIS_GRAPHS_HASHMAP_HPP_

#include <ctype.h>
#include <pando-rt/export.h>
#include <stdlib.h>

#include <cassert>
#include <iostream>
#include <numeric> // std::iota
#include <utility>
#include <vector>

#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/global_ptr.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/notification.hpp>

// Chaining to handle hash collisions
template <typename VT>
struct KVPair {
  int64_t key;
  VT value;
};

template <typename T>
struct Optional {
  T item;
  bool is_valid = false;
};

// Assumes KT = int64_t
template <typename VT>
struct HashMap {
  using KVPairVectorPando = pando::Vector<KVPair<VT>>;
  pando::GlobalPtr<KVPairVectorPando> buckets_ptr = nullptr;
  size_t num_buckets = 0;

  void initialize(size_t num_b) {
    num_buckets = num_b;
    buckets_ptr = static_cast<pando::GlobalPtr<KVPairVectorPando>>(
        pando::getDefaultMainMemoryResource()->allocate(sizeof(KVPairVectorPando) * num_b));
    for (size_t i = 0; i < num_buckets; i++) {
      KVPairVectorPando bucket = buckets_ptr[i];
      PANDO_CHECK(bucket.initialize(0));
      buckets_ptr[i] = std::move(bucket);
    }
  }

  void deinitialize() {
    if (buckets_ptr != nullptr)
      pando::deallocateMemory(buckets_ptr, num_buckets);
    buckets_ptr = nullptr;
  }

  int64_t hash(int64_t k) {
    return k % num_buckets;
  }

  void insert(int64_t key, VT value) {
    auto indx = hash(key);
    KVPairVectorPando bucket = buckets_ptr[indx];
    KVPair<VT> new_pair = KVPair<VT>{key, value};
    PANDO_CHECK(bucket.pushBack(new_pair));
    buckets_ptr[indx] = std::move(bucket);
  }

  bool check_existence(int64_t k) {
    auto indx = hash(k);
    KVPairVectorPando bucket = buckets_ptr[indx];
    for (size_t i = 0; i < bucket.size(); i++) {
      KVPair<VT> pair = bucket[i];
      if (pair.key == k)
        return true;
    }
    return false;
  }

  Optional<VT> lookup(int64_t k) {
    auto indx = hash(k);
    KVPairVectorPando bucket = buckets_ptr[indx];
    for (size_t i = 0; i < bucket.size(); i++) {
      KVPair<VT> pair = bucket[i];
      if (pair.key == k)
        return Optional<VT>{pair.value, true};
    }
    return Optional<VT>{VT(), false};
  }
};
#endif // PANDO_LIB_GALOIS_GRAPHS_HASHMAP_HPP_
