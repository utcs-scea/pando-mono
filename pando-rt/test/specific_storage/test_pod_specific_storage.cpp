// SPDX-License-Identifier: MIT
/* Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include <cstdint>
#include <random>

#include "pando-rt/specific_storage.hpp"

#include "pando-rt/execution/execute_on_wait.hpp"
#include "pando-rt/locality.hpp"
#include "pando-rt/memory/allocate_memory.hpp"
#include "pando-rt/memory/global_ptr.hpp"
#include "pando-rt/sync/atomic.hpp"

namespace {

pando::PodSpecificStorage<std::int32_t> globalI32;
pando::PodSpecificStorage<bool> globalBool;
pando::PodSpecificStorage<std::int64_t> globalI64;

// Executes f once at each pod. This is serializing intentionally for testing purposes.
template <typename F, typename... Args>
[[nodiscard]] auto onAllPods(F&& f, Args&&... args) {
  for (auto nodeIdx = 0; nodeIdx < pando::getNodeDims().id; ++nodeIdx) {
    for (auto podX = 0; podX < pando::getPodDims().x; ++podX) {
      for (auto podY = 0; podY < pando::getPodDims().y; ++podY) {
        const pando::Place place{pando::NodeIndex(nodeIdx), pando::PodIndex(podX, podY),
                                 pando::anyCore};
        auto result = pando::executeOnWait(place, f, args...);
        if (!result.hasValue()) {
          return testing::AssertionFailure()
                 << "executeOnWait() failed for (" << nodeIdx << ", (" << podX << ',' << podY
                 << ")): " << pando::errorString(result.error());
        }
      }
    }
  }
  return testing::AssertionSuccess();
}

inline std::uint64_t globalPodIndex() {
  const auto podDims = pando::getPodDims();
  const auto podsPerNode = podDims.x * podDims.y;
  const auto thisPlace = pando::getCurrentPlace();
  return thisPlace.node.id * podsPerNode + thisPlace.pod.x * podDims.y + thisPlace.pod.y;
}

} // namespace

TEST(PodSpecificStorage, GetPointer) {
  auto f = +[] {
    EXPECT_EQ(globalI32.getPointer(), &globalI32);
    EXPECT_EQ(globalBool.getPointer(), &globalBool);
    EXPECT_EQ(globalI64.getPointer(), &globalI64);
  };

  EXPECT_TRUE(onAllPods(f));
}

TEST(PodSpecificStorage, NoAliasing) {
  using BytePtr = pando::GlobalPtr<std::byte>;

  auto f = +[] {
    EXPECT_LE(pando::globalPtrReinterpretCast<BytePtr>(&globalI32) + sizeof(std::int32_t),
              pando::globalPtrReinterpretCast<BytePtr>(&globalBool));
    EXPECT_LE(pando::globalPtrReinterpretCast<BytePtr>(&globalBool) + sizeof(bool),
              pando::globalPtrReinterpretCast<BytePtr>(&globalI64));
  };

  EXPECT_TRUE(onAllPods(f));
}

TEST(PodSpecificStorage, Locality) {
  auto f = +[] {
    const auto thisPod =
        pando::Place{pando::getCurrentNode(), pando::getCurrentPod(), pando::anyCore};
    EXPECT_EQ(pando::localityOf(&globalI32), thisPod);
    EXPECT_EQ(pando::localityOf(&globalBool), thisPod);
    EXPECT_EQ(pando::localityOf(&globalI64), thisPod);
  };

  EXPECT_TRUE(onAllPods(f));
}

TEST(PodSpecificStorage, GetPointerAt) {
  for (auto nodeIdx = 0; nodeIdx < pando::getNodeDims().id; ++nodeIdx) {
    for (auto podX = 0; podX < pando::getPodDims().x; ++podX) {
      for (auto podY = 0; podY < pando::getPodDims().y; ++podY) {
        const pando::NodeIndex node(nodeIdx);
        const pando::PodIndex pod(podX, podY);
        const pando::Place place{node, pod, pando::anyCore};
        EXPECT_EQ(pando::localityOf(globalI32.getPointerAt(node, pod)), place);
        EXPECT_EQ(pando::localityOf(globalBool.getPointerAt(node, pod)), place);
        EXPECT_EQ(pando::localityOf(globalI64.getPointerAt(node, pod)), place);
      }
    }
  }
}

TEST(PodSpecificStorage, WriteRead) {
  auto f = +[] {
    std::minstd_rand rng(globalPodIndex());
    const auto value = rng();
    globalI32 = value;
    EXPECT_EQ(globalI32, value);
  };

  EXPECT_TRUE(onAllPods(f));
}

TEST(PodSpecificStorage, WriteReadSplit) {
  auto writeF = +[] {
    std::minstd_rand rng(globalPodIndex());
    globalI32 = rng();
  };
  EXPECT_TRUE(onAllPods(writeF));

  auto readF = +[] {
    std::minstd_rand rng(globalPodIndex());
    EXPECT_EQ(globalI32, rng());
  };

  EXPECT_TRUE(onAllPods(readF));
}

TEST(PodSpecificStorage, RemotePodGet) {
  const std::int32_t value = 786534231;
  const auto rightNodeIdx = (pando::getCurrentNode().id + 1) % pando::getNodeDims().id;
  const pando::Place place{pando::NodeIndex(rightNodeIdx), pando::PodIndex(0, 0), pando::anyCore};

  // write value
  {
    auto f = +[](std::int32_t value) {
      globalI32 = value + pando::getCurrentNode().id;
    };
    auto result = pando::executeOnWait(place, f, value);
    EXPECT_TRUE(result.hasValue());
  }

  // read value
  {
    auto f = +[](std::int32_t value) {
      EXPECT_EQ(globalI32, value + pando::getCurrentNode().id);
    };
    auto result = pando::executeOnWait(place, f, value);
    EXPECT_TRUE(result.hasValue());
  }
}

TEST(PodSpecificStorage, RemoteWriteLocalWriteBothRead) {
  const auto err = pando::executeOnWait(
      pando::Place{pando::NodeIndex{0}, pando::PodIndex{0, 0}, pando::anyCore}, +[]() {
        constexpr std::int64_t v0 = 0x00000000;
        constexpr std::int64_t v1 = 0xDEADBEEF;
        const auto nodes = pando::getPlaceDims().node.id;
        const auto podx = pando::getPlaceDims().pod.x;
        const auto pody = pando::getPlaceDims().pod.y;
        for (std::int16_t i = 0; i < nodes * podx * pody; i++) {
          const auto node = pando::NodeIndex(i / (podx * pody));
          const auto podIdx = i % (podx * pody);
          const auto pod = pando::PodIndex(podIdx / podx, podIdx % podx);
          *globalI64.getPointerAt(node, pod) = v1;
          *globalI64.getPointer() = v0;
          EXPECT_EQ(*globalI64.getPointerAt(pando::NodeIndex{0}, pando::PodIndex{0, 0}), v0);
          if (node != pando::NodeIndex{0} || pod != pando::PodIndex{0, 0}) {
            EXPECT_EQ(*globalI64.getPointerAt(node, pod), v1);
          }
        }
        return true;
      });
  EXPECT_TRUE(err);
}

TEST(PodSpecificStorageAlias, ExecuteOn) {
  constexpr std::int64_t NUM = 10;
  pando::PodSpecificStorageAlias<std::int64_t> gI64(globalI64);
  auto f = +[](pando::PodSpecificStorageAlias<std::int64_t> p, std::int64_t NUM) {
    *p.getPointer() = NUM;
  };
  for (std::int16_t i = 0; i < pando::getPlaceDims().node.id; i++) {
    for (std::int16_t j = 0; j < pando::getPlaceDims().pod.x * pando::getPlaceDims().pod.y; j++) {
      const pando::PodIndex pod = pando::PodIndex(
          static_cast<std::int8_t>(j / static_cast<std::int16_t>(pando::getPlaceDims().pod.x)),
          static_cast<std::int8_t>(j % static_cast<std::int16_t>(pando::getPlaceDims().pod.x)));
      const pando::Place place{pando::NodeIndex(i), pod, pando::anyCore};
      auto result = pando::executeOnWait(place, f, gI64, NUM);
    }
  }

  for (std::int16_t i = 0; i < pando::getPlaceDims().node.id; i++) {
    for (std::int16_t j = 0; j < pando::getPlaceDims().pod.x * pando::getPlaceDims().pod.y; j++) {
      const pando::PodIndex pod = pando::PodIndex(
          static_cast<std::int8_t>(j / static_cast<std::int16_t>(pando::getPlaceDims().pod.x)),
          static_cast<std::int8_t>(j % static_cast<std::int16_t>(pando::getPlaceDims().pod.x)));
      pando::GlobalPtr<const std::int64_t> ptr = gI64.getPointerAt(pando::NodeIndex{i}, pod);
      EXPECT_EQ(NUM, *ptr);
    }
  }
}

TEST(PodSpecificStorageAlias, SlicingFail) {
  pando::PodSpecificStorageAlias<std::int64_t> gI64(globalI64);
  for (std::int16_t i = 0; i < pando::getPlaceDims().node.id; i++) {
    for (std::int16_t j = 0; j < pando::getPlaceDims().pod.x * pando::getPlaceDims().pod.y; j++) {
      const pando::PodIndex pod = pando::PodIndex(
          static_cast<std::int8_t>(j / static_cast<std::int16_t>(pando::getPlaceDims().pod.x)),
          static_cast<std::int8_t>(j % static_cast<std::int16_t>(pando::getPlaceDims().pod.x)));
      auto place = pando::Place{pando::NodeIndex(i), pod, pando::anyCore};
      auto expectPtr = pando::allocateMemory<std::int16_t>(1, place, pando::MemoryType::L2SP);
      EXPECT_TRUE(expectPtr.hasValue());
      pando::GlobalPtr<std::int16_t> ptr = expectPtr.value();
      auto expected = gI64.getStorageAliasAt(ptr);
      EXPECT_FALSE(expected.hasValue());
      EXPECT_EQ(expected.error(), pando::Status::OutOfBounds);
      pando::deallocateMemory(ptr, 1);
    }
  }
}

TEST(PodSpecificStorageAlias, SlicingSuccess) {
  pando::PodSpecificStorageAlias<std::int64_t> gI64(globalI64);

  const auto nodes = pando::getPlaceDims().node.id;
  const auto podx = pando::getPlaceDims().pod.x;
  const auto pody = pando::getPlaceDims().pod.y;
  const std::int16_t localPods = podx * pody;
  const std::int32_t totalPods = nodes * localPods;
  for (std::int32_t i = 0; i < totalPods; i++) {
    const pando::NodeIndex nodei = pando::NodeIndex(i / localPods);
    const auto podIdxi = i % localPods;
    const pando::PodIndex podi = pando::PodIndex(podIdxi / podx, podIdxi % podx);
    pando::GlobalPtr<void> ptrVoid =
        static_cast<pando::GlobalPtr<void>>(gI64.getPointerAt(nodei, podi));
    pando::GlobalPtr<std::int16_t> ptr = static_cast<pando::GlobalPtr<std::int16_t>>(ptrVoid);
    auto expected = gI64.getStorageAliasAt(ptr);
    ASSERT_TRUE(expected.hasValue());
    pando::PodSpecificStorageAlias<std::int16_t> gI16AliasI64 = expected.value();
    for (std::int16_t j = 0; j < totalPods; j++) {
      const pando::NodeIndex nodej = pando::NodeIndex(j / localPods);
      const auto podIdxj = j % localPods;
      const pando::PodIndex podj = pando::PodIndex(podIdxj / podx, podIdxj % podx);
      if (i == 0) {
        *gI16AliasI64.getPointerAt(nodej, podj) = 0;
      } else if (i == j) {
        continue;
      } else {
        *gI16AliasI64.getPointerAt(nodej, podj) += i;
      }
    }
  }
  std::int16_t sum = 0;
  for (std::int16_t j = 1; j < totalPods; j++) {
    sum += j;
  }

  for (std::int16_t j = 0; j < totalPods; j++) {
    const pando::NodeIndex nodej = pando::NodeIndex(j / localPods);
    const auto podIdxj = j % localPods;
    const pando::PodIndex podj = pando::PodIndex(podIdxj / podx, podIdxj % podx);
    pando::GlobalPtr<std::int16_t> ptr = static_cast<pando::GlobalPtr<std::int16_t>>(
        static_cast<pando::GlobalPtr<void>>(globalI64.getPointerAt(nodej, podj)));
    EXPECT_EQ(*ptr, sum - j);
  }
}
