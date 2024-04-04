// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <pando-lib-galois/containers/array.hpp>
#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/containers/per_thread.hpp>
#include <pando-lib-galois/utility/prefix_sum.hpp>

namespace {

template <typename A>
uint64_t transmute(A p) {
  return p;
}
template <typename A, typename B>
uint64_t scan_op(A p, B l) {
  return p + l;
}
template <typename B>
uint64_t combiner(B f, B s) {
  return f + s;
}
uint64_t transmuteV(pando::Vector<uint64_t> p) {
  return p.size();
}
uint64_t scan_opV(pando::Vector<uint64_t> p, uint64_t l) {
  return p.size() + l;
}

} // namespace

TEST(PrefixSum, Init) {
  const uint64_t elts = 100;
  galois::DistArray<uint64_t> arr;
  EXPECT_EQ(arr.initialize(elts), pando::Status::Success);
  galois::DistArray<uint64_t> prefixArr;
  EXPECT_EQ(prefixArr.initialize(elts), pando::Status::Success);
  for (uint64_t i = 0; i < arr.size(); i++) {
    arr[i] = i;
  }

  using SRC = galois::DistArray<uint64_t>;
  using DST = galois::DistArray<uint64_t>;
  using SRC_Val = uint64_t;
  using DST_Val = uint64_t;

  galois::PrefixSum<SRC, DST, SRC_Val, DST_Val, transmute<uint64_t>, scan_op<SRC_Val, DST_Val>,
                    combiner<DST_Val>, galois::DistArray>
      prefixSum(arr, prefixArr);
  EXPECT_EQ(prefixSum.initialize(pando::getPlaceDims().node.id), pando::Status::Success);
  prefixSum.computePrefixSum(elts);

  uint64_t expected = 0;
  for (uint64_t i = 0; i < prefixArr.size(); i++) {
    uint64_t val = prefixArr[i];
    expected += i;
    EXPECT_EQ(val, expected);
  }
  prefixSum.deinitialize();
}

TEST(PrefixSum, PerThread) {
  galois::PerThreadVector<uint64_t> arr;
  EXPECT_EQ(arr.initialize(), pando::Status::Success);
  galois::DistArray<uint64_t> prefixArr;
  EXPECT_EQ(prefixArr.initialize(arr.size()), pando::Status::Success);

  using SRC = galois::DistArray<pando::Vector<uint64_t>>;
  using DST = galois::DistArray<uint64_t>;
  using SRC_Val = pando::Vector<uint64_t>;
  using DST_Val = uint64_t;

  galois::PrefixSum<SRC, DST, SRC_Val, DST_Val, transmuteV, scan_opV, combiner<DST_Val>,
                    galois::DistArray>
      prefixSum(arr.m_data, prefixArr);
  EXPECT_EQ(prefixSum.initialize(pando::getPlaceDims().node.id), pando::Status::Success);
  prefixSum.computePrefixSum(prefixArr.size());
  EXPECT_EQ(prefixArr[prefixArr.size() - 1], arr.sizeAll());
}

TEST(PrefixSum, Array) {
  constexpr std::uint64_t size = 1000;
  galois::Array<std::uint64_t> arr;
  PANDO_CHECK(arr.initialize(size));
  for (std::uint64_t i = 0; i < size; i++) {
    arr[i] = 1;
  }

  using SRC = galois::Array<uint64_t>;
  using DST = galois::Array<uint64_t>;
  using SRC_VAL = uint64_t;
  using DST_VAL = uint64_t;
  using PFXSUM = galois::PrefixSum<SRC, DST, SRC_VAL, DST_VAL, transmute<std::uint64_t>,
                                   scan_op<SRC_VAL, DST_VAL>, combiner<DST_VAL>, galois::Array>;
  PFXSUM pfxsum(arr, arr);

  PANDO_CHECK(pfxsum.initialize(pando::getPlaceDims().core.x * pando::getPlaceDims().core.y));

  pfxsum.computePrefixSum(size);

  for (std::uint64_t i = 0; i < size; i++) {
    EXPECT_EQ(arr[i], i + 1) << "The arrays are not equal @" << i << "v: \t" << arr[i] << std::endl;
  }
  pfxsum.deinitialize();
  arr.deinitialize();
}
