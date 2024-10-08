// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <pando-lib-galois/graphs/dist_array_csr.hpp>
#include <pando-lib-galois/utility/gptr_monad.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/memory_guard.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/utility/expected.hpp>

struct TestEdgeType {
  uint64_t dst = 0;
};

using Graph = galois::DistArrayCSR<uint64_t, TestEdgeType>;

TEST(Fmap, GVectorInitialize) {
  constexpr std::uint64_t SIZE = 10;
  pando::GlobalPtr<pando::Vector<std::uint64_t>> gvec;
  auto expect = pando::allocateMemory<pando::Vector<std::uint64_t>>(1, pando::getCurrentPlace(),
                                                                    pando::MemoryType::Main);
  if (!expect.hasValue()) {
    PANDO_CHECK(expect.error());
  }
  gvec = expect.value();
  fmap(*gvec, initialize, SIZE);
  pando::Vector<std::uint64_t> vec = *gvec;
  EXPECT_EQ(vec.size(), SIZE);
  vec.deinitialize();
  pando::deallocateMemory(gvec, 1);
}

TEST(Fmap, VectorInitialize) {
  constexpr std::uint64_t SIZE = 10;
  pando::Vector<std::uint64_t> vec;
  fmap(vec, initialize, SIZE);
  EXPECT_EQ(vec.size(), SIZE);
  vec.deinitialize();
}

TEST(Fmap, GVectorPushBack) {
  constexpr std::uint64_t SIZE = 10;
  pando::GlobalPtr<pando::Vector<std::uint64_t>> gvec;
  auto expect = pando::allocateMemory<pando::Vector<std::uint64_t>>(1, pando::getCurrentPlace(),
                                                                    pando::MemoryType::Main);
  if (!expect.hasValue()) {
    PANDO_CHECK(expect.error());
  }
  gvec = expect.value();
  PANDO_CHECK(fmap(*gvec, initialize, 0));

  for (std::uint64_t i = 0; i < SIZE; i++) {
    PANDO_CHECK(fmap(*gvec, pushBack, i));
  }

  pando::Vector<std::uint64_t> vec = *gvec;
  EXPECT_EQ(vec.size(), SIZE);
  std::uint64_t i = 0;
  for (std::uint64_t v : vec) {
    EXPECT_EQ(v, i);
    i++;
  }
  vec.deinitialize();
  EXPECT_EQ(SIZE, i);
  pando::deallocateMemory(gvec, 1);
}

TEST(Fmap, VectorPushBack) {
  constexpr std::uint64_t SIZE = 10;
  pando::Vector<std::uint64_t> vec;
  PANDO_CHECK(fmap(vec, initialize, 0));

  for (std::uint64_t i = 0; i < SIZE; i++) {
    PANDO_CHECK(fmap(vec, pushBack, i));
  }

  EXPECT_EQ(vec.size(), SIZE);
  std::uint64_t i = 0;
  for (std::uint64_t v : vec) {
    EXPECT_EQ(v, i);
    i++;
  }
  vec.deinitialize();
  EXPECT_EQ(SIZE, i);
}

pando::Vector<pando::Vector<TestEdgeType>> generateFullyConnectedGraph(std::uint64_t SIZE) {
  pando::Vector<pando::Vector<TestEdgeType>> vec;
  EXPECT_EQ(vec.initialize(SIZE), pando::Status::Success);
  for (pando::GlobalRef<pando::Vector<TestEdgeType>> edges : vec) {
    pando::Vector<TestEdgeType> inner;
    EXPECT_EQ(inner.initialize(0), pando::Status::Success);
    edges = inner;
  }

  galois::doAll(
      SIZE, vec, +[](std::uint64_t size, pando::GlobalRef<pando::Vector<TestEdgeType>> innerRef) {
        pando::Vector<TestEdgeType> inner = innerRef;
        for (std::uint64_t i = 0; i < size; i++) {
          EXPECT_EQ(inner.pushBack(TestEdgeType{i}), pando::Status::Success);
        }
        innerRef = inner;
      });
  return vec;
}

template <typename T>
pando::Status deleteVectorVector(pando::Vector<pando::Vector<T>> vec) {
  auto err = galois::doAll(
      vec, +[](pando::GlobalRef<pando::Vector<T>> innerRef) {
        pando::Vector<T> inner = innerRef;
        inner.deinitialize();
        innerRef = inner;
      });
  vec.deinitialize();
  return err;
}

TEST(FmapVoid, GDistArrayCSR) {
  constexpr std::uint64_t SIZE = 10;
  pando::GlobalPtr<Graph> ggraph;
  pando::LocalStorageGuard ggraphGuard(ggraph, 1);
  *ggraph = Graph();
  auto vvec = generateFullyConnectedGraph(SIZE);
  PANDO_CHECK(fmap(*ggraph, initialize, vvec));
  PANDO_CHECK(deleteVectorVector(vvec));

  for (std::uint64_t i = 0; i < SIZE; i++) {
    fmapVoid(*ggraph, setData, i, i);
    for (std::uint64_t j = 0; j < SIZE; j++) {
      fmapVoid(*ggraph, setEdgeData, i, j, TestEdgeType{i * j});
    }
  }

  for (std::uint64_t i = 0; i < SIZE; i++) {
    EXPECT_EQ(fmap(*ggraph, getData, i), i);
    for (std::uint64_t j = 0; j < SIZE; j++) {
      TestEdgeType actual = fmap(*ggraph, getEdgeData, i, j);
      EXPECT_EQ(actual.dst, i * j);
    }
  }
  liftVoid(*ggraph, deinitialize);
}

TEST(FmapVoid, DistArrayCSR) {
  constexpr std::uint64_t SIZE = 10;
  Graph graph{};
  auto vvec = generateFullyConnectedGraph(SIZE);
  PANDO_CHECK(fmap(graph, initialize, vvec));
  PANDO_CHECK(deleteVectorVector(vvec));

  for (std::uint64_t i = 0; i < SIZE; i++) {
    fmapVoid(graph, setData, i, i);
    for (std::uint64_t j = 0; j < SIZE; j++) {
      fmapVoid(graph, setEdgeData, i, j, TestEdgeType{i * j});
    }
  }

  for (std::uint64_t i = 0; i < SIZE; i++) {
    EXPECT_EQ(fmap(graph, getData, i), i);
    for (std::uint64_t j = 0; j < SIZE; j++) {
      TestEdgeType actual = fmap(graph, getEdgeData, i, j);
      EXPECT_EQ(actual.dst, i * j);
    }
  }
  liftVoid(graph, deinitialize);
}

TEST(Lift, GVectorSize) {
  constexpr std::uint64_t SIZE = 10;
  pando::GlobalPtr<pando::Vector<std::uint64_t>> gvec;
  auto expect = pando::allocateMemory<pando::Vector<std::uint64_t>>(1, pando::getCurrentPlace(),
                                                                    pando::MemoryType::Main);
  if (!expect.hasValue()) {
    PANDO_CHECK(expect.error());
  }
  gvec = expect.value();
  PANDO_CHECK(fmap(*gvec, initialize, SIZE));
  EXPECT_EQ(lift(*gvec, size), SIZE);
  pando::Vector<std::uint64_t> vec = *gvec;
  vec.deinitialize();

  pando::deallocateMemory(gvec, 1);
}

TEST(Lift, VectorSize) {
  constexpr std::uint64_t SIZE = 10;
  pando::Vector<std::uint64_t> vec;
  PANDO_CHECK(fmap(vec, initialize, SIZE));
  EXPECT_EQ(lift(vec, size), SIZE);
  vec.deinitialize();
}

TEST(LiftVoid, GVectorDeinitialize) {
  constexpr std::uint64_t SIZE = 10;
  pando::GlobalPtr<pando::Vector<std::uint64_t>> gvec;
  auto expect = pando::allocateMemory<pando::Vector<std::uint64_t>>(1, pando::getCurrentPlace(),
                                                                    pando::MemoryType::Main);
  if (!expect.hasValue()) {
    PANDO_CHECK(expect.error());
  }
  gvec = expect.value();
  PANDO_CHECK(fmap(*gvec, initialize, SIZE));
  EXPECT_EQ(lift(*gvec, size), SIZE);
  liftVoid(*gvec, deinitialize);
  pando::deallocateMemory(gvec, 1);
}

TEST(LiftVoid, VectorDeinitialize) {
  constexpr std::uint64_t SIZE = 10;
  pando::Vector<std::uint64_t> vec;
  PANDO_CHECK(fmap(vec, initialize, SIZE));
  EXPECT_EQ(lift(vec, size), SIZE);
  liftVoid(vec, deinitialize);
}

TEST(PANDO_EXPECT_RETURN, Success) {
  auto success = +[]() -> pando::Status {
    const std::int32_t value = 42;

    std::int32_t v = PANDO_EXPECT_RETURN(pando::Expected<std::int32_t>(value));
    EXPECT_EQ(v, value);
    return pando::Status::Error;
  };
  EXPECT_EQ(pando::Status::Error, success());
}

TEST(PANDO_EXPECT_RETURN, Fail) {
  auto returnFailure = +[]() -> pando::Status {
    const auto value = pando::Status::NotImplemented;

    std::int32_t v = PANDO_EXPECT_RETURN(pando::Expected<std::int32_t>(value));
    EXPECT_TRUE(false) << "Should not have gotten here";
    EXPECT_EQ(v, 100);
    return pando::Status::Error;
  };
  EXPECT_EQ(pando::Status::NotImplemented, returnFailure());
}
