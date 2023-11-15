// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <random>

#include "pando-rt/export.h"
#include <pando-lib-galois/containers/hashtable.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/notification.hpp>

template <typename Key, typename T>
void checkCorrectness(galois::HashTable<Key, T>& table) {
  using Entry = typename galois::HashTable<Key, T>::Entry;

  EXPECT_GT(table.capacity(), table.size());

  for (Entry e : table) {
    T tmp;
    EXPECT_EQ(e.occupied, true);
    EXPECT_EQ(table.get(e.key, tmp), true);
    EXPECT_EQ(tmp, e.value);
  }
}

TEST(HashTable, Empty) {
  galois::HashTable<int, int> table;

  EXPECT_EQ(table.size(), 0);
  EXPECT_EQ(table.capacity(), 0);
}

TEST(HashTable, Initialize) {
  galois::HashTable<int, int> table;

  EXPECT_EQ(table.initialize(8), pando::Status::Success);
  EXPECT_EQ(table.capacity(), 8);
}

TEST(HashTable, Resize) {
  galois::HashTable<int, int> table;

  EXPECT_EQ(table.initialize(8), pando::Status::Success);
  table.put(1, 1);
  table.put(2, 2);
  table.put(3, 3);
  table.put(4, 4);
  checkCorrectness(table);
  table.put(5, 5);
  table.put(6, 6);
  table.put(7, 7);
  table.put(8, 8);
  checkCorrectness(table);

  EXPECT_GT(table.capacity(), 8);
}

TEST(HashTable, PutGet) {
  galois::HashTable<int, int> table;

  EXPECT_EQ(table.initialize(8), pando::Status::Success);
  table.put(1, 1);
  table.put(2, 2);
  table.put(3, 3);
  checkCorrectness(table);
  table.put(4, 4);
  table.put(5, 5);
  table.put(6, 6);
  checkCorrectness(table);
  table.put(7, 7);
  table.put(8, 8);

  for (int i = 1; i <= 8; i++) {
    int val;
    EXPECT_EQ(table.get(i, val), true);
    EXPECT_EQ(val, i);
  }
}

TEST(HashTable, PutGetResize) {
  int seed = 0;
  std::minstd_rand0 gen(seed);
  int arr[900];
  for (int i = 0; i < 900; i++) {
    arr[i] = gen();
  }

  galois::HashTable<int, int> table;

  EXPECT_EQ(table.initialize(1), pando::Status::Success);

  for (int i = 0; i < 900; i++) {
    table.put(i, arr[i]);
  }

  for (int i = 0; i < 900; i++) {
    int val;
    EXPECT_EQ(table.get(i, val), true);
    EXPECT_EQ(val, arr[i]);
  }
}

TEST(HashTable, PutGet900) {
  int seed = 0;
  std::minstd_rand0 gen(seed);
  int arr[900];
  for (int i = 0; i < 900; i++) {
    arr[i] = gen();
  }

  galois::HashTable<int, int> table;

  EXPECT_EQ(table.initialize(1024), pando::Status::Success);

  for (int i = 0; i < 900; i++) {
    table.put(i, arr[i]);
  }

  checkCorrectness(table);

  for (int i = 0; i < 900; i++) {
    int val;
    EXPECT_EQ(table.get(i, val), true);
    EXPECT_EQ(val, arr[i]);
  }

  checkCorrectness(table);
}

TEST(HashTable, Clear) {
  galois::HashTable<int, int> table;

  EXPECT_EQ(table.initialize(8), pando::Status::Success);
  table.put(1, 1);
  table.put(2, 2);
  table.put(3, 3);
  table.put(4, 4);
  table.put(5, 5);
  table.put(6, 6);
  table.put(7, 7);
  table.put(8, 8);
  checkCorrectness(table);

  table.clear();
  checkCorrectness(table);
  EXPECT_EQ(table.size(), 0);
  EXPECT_GT(table.capacity(), 8);
}

TEST(HashTable, GetFail) {
  galois::HashTable<int, int> table;

  EXPECT_EQ(table.initialize(8), pando::Status::Success);
  table.put(1, 1);
  table.put(2, 2);
  table.put(3, 3);
  table.put(4, 4);
  table.put(5, 5);
  table.put(6, 6);
  table.put(7, 7);
  table.put(8, 8);

  checkCorrectness(table);
  for (int i = 9; i < 100; i++) {
    int _tmp;
    EXPECT_EQ(table.get(i, _tmp), false);
    checkCorrectness(table);
  }
}
