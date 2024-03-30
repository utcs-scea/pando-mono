// SPDX-License-Identifier: MIT
/* Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include <cstdint>

#include "pando-rt/specific_storage.hpp"

#include "pando-rt/execution/execute_on_wait.hpp"
#include "pando-rt/locality.hpp"
#include "pando-rt/memory/allocate_memory.hpp"
#include "pando-rt/memory/global_ptr.hpp"
#include "pando-rt/sync/atomic.hpp"

namespace {

pando::NodeSpecificStorage<std::int32_t> globalI32;
pando::NodeSpecificStorage<bool> globalBool;
pando::NodeSpecificStorage<std::int64_t> globalI64;

// Executes f once at each node. This is serializing intentionally for testing purposes.
template <typename F, typename... Args>
[[nodiscard]] auto onAllNodes(F&& f, Args&&... args) {
  for (auto nodeIdx = 0; nodeIdx < pando::getNodeDims().id; ++nodeIdx) {
    const pando::Place place{pando::NodeIndex(nodeIdx), pando::anyPod, pando::anyCore};
    auto result = pando::executeOnWait(place, f, args...);
    if (!result.hasValue()) {
      return testing::AssertionFailure() << "executeOnWait() failed for (" << nodeIdx
                                         << "): " << pando::errorString(result.error());
    }
  }
  return testing::AssertionSuccess();
}

} // namespace

TEST(NodeSpecificStorage, GetPointerFromCP) {
  EXPECT_EQ(globalI32.getPointer(), &globalI32);
  EXPECT_EQ(globalBool.getPointer(), &globalBool);
  EXPECT_EQ(globalI64.getPointer(), &globalI64);
}

TEST(NodeSpecificStorage, GetPointer) {
  auto f = +[] {
    EXPECT_EQ(globalI32.getPointer(), &globalI32);
    EXPECT_EQ(globalBool.getPointer(), &globalBool);
    EXPECT_EQ(globalI64.getPointer(), &globalI64);
  };

  EXPECT_TRUE(onAllNodes(f));
}

TEST(NodeSpecificStorage, NoAliasing) {
  using BytePtr = pando::GlobalPtr<std::byte>;

  EXPECT_LE(pando::globalPtrReinterpretCast<BytePtr>(&globalI32) + sizeof(std::int32_t),
            pando::globalPtrReinterpretCast<BytePtr>(&globalBool));
  EXPECT_LE(pando::globalPtrReinterpretCast<BytePtr>(&globalBool) + sizeof(bool),
            pando::globalPtrReinterpretCast<BytePtr>(&globalI64));
}

TEST(NodeSpecificStorage, Locality) {
  auto f = +[] {
    const auto thisNode = pando::Place{pando::getCurrentNode(), pando::anyPod, pando::anyCore};
    EXPECT_EQ(pando::localityOf(&globalI32), thisNode);
    EXPECT_EQ(pando::localityOf(&globalBool), thisNode);
    EXPECT_EQ(pando::localityOf(&globalI64), thisNode);
  };

  EXPECT_TRUE(onAllNodes(f));
}

TEST(NodeSpecificStorage, GetPointerAt) {
  for (auto nodeIdx = 0; nodeIdx < pando::getNodeDims().id; ++nodeIdx) {
    const pando::NodeIndex node(nodeIdx);
    const pando::Place place{node, pando::anyPod, pando::anyCore};
    EXPECT_EQ(pando::localityOf(globalI32.getPointerAt(node)), place);
    EXPECT_EQ(pando::localityOf(globalBool.getPointerAt(node)), place);
    EXPECT_EQ(pando::localityOf(globalI64.getPointerAt(node)), place);
  }
}

TEST(NodeSpecificStorage, WriteReadFromCP) {
  const std::int32_t value = 1234;

  globalI32 = value;
  EXPECT_EQ(globalI32, value);
}

TEST(NodeSpecificStorage, WriteFromCPReadFromCore) {
  const std::int32_t value = 987654321;

  globalI32 = value;
  EXPECT_EQ(globalI32, value);

  auto f = +[](std::int32_t value) {
    EXPECT_EQ(globalI32, value);
  };

  const pando::Place place{pando::getCurrentNode(), pando::anyPod, pando::anyCore};
  auto result = pando::executeOnWait(place, f, value);
  EXPECT_TRUE(result.hasValue());
}

TEST(NodeSpecificStorage, WriteFromCoreReadFromCP) {
  const std::int32_t value = 987654321;

  auto f = +[](std::int32_t value) {
    pando::atomicStore(globalI32.getPointer(), value, std::memory_order_relaxed);
    EXPECT_EQ(globalI32, value);
  };

  const pando::Place place{pando::getCurrentNode(), pando::anyPod, pando::anyCore};
  auto result = pando::executeOnWait(place, f, value);
  EXPECT_TRUE(result.hasValue());

  EXPECT_EQ(globalI32, value);
}

TEST(NodeSpecificStorage, ReadFromRemoteNode) {
  const std::int32_t value = 786534231;

  globalI32 = value;
  EXPECT_EQ(globalI32, value);

  auto f = +[] {
    EXPECT_EQ(globalI32, 0);
  };

  const auto rightNodeIdx = (pando::getCurrentNode().id + 1) % pando::getNodeDims().id;
  const pando::Place place{pando::NodeIndex(rightNodeIdx), pando::anyPod, pando::anyCore};
  auto result = pando::executeOnWait(place, f);
  EXPECT_TRUE(result.hasValue());
}

TEST(NodeSpecificStorage, WriteToRemoteNode) {
  const std::int32_t value = 786534231;

  globalI32 = value + 1;
  EXPECT_EQ(globalI32, value + 1);

  const auto rightNodeIdx = (pando::getCurrentNode().id + 1) % pando::getNodeDims().id;
  const pando::Place place{pando::NodeIndex(rightNodeIdx), pando::anyPod, pando::anyCore};
  {
    auto f = +[](std::int32_t value) {
      globalI32 = value;
      EXPECT_EQ(globalI32, value);
    };

    auto result = pando::executeOnWait(place, f, value);
    EXPECT_TRUE(result.hasValue());
  }

  EXPECT_EQ(globalI32, value + 1);

  {
    auto f = +[](std::int32_t value) {
      EXPECT_EQ(globalI32, value);
    };

    auto result = pando::executeOnWait(place, f, value);
    EXPECT_TRUE(result.hasValue());
  }
}

TEST(NodeSpecificStorage, ExecuteOn) {
  constexpr std::int64_t NUM = 10;
  pando::NodeSpecificStorageAlias<std::int64_t> gI64(globalI64);
  auto f = +[](pando::NodeSpecificStorageAlias<std::int64_t> p, std::int64_t NUM) {
    *p.getPointer() = NUM;
  };
  for (std::int16_t i = 0; i < pando::getPlaceDims().node.id; i++) {
    const pando::Place place{pando::NodeIndex(i), pando::anyPod, pando::anyCore};
    auto result = pando::executeOnWait(place, f, gI64, NUM);
  }

  for (std::int16_t i = 0; i < pando::getPlaceDims().node.id; i++) {
    pando::GlobalPtr<const std::int64_t> ptr = gI64.getPointerAt(pando::NodeIndex{i});
    EXPECT_EQ(NUM, *ptr);
  }
}

TEST(NodeSpecificStorageAlias, SlicingFail) {
  pando::NodeSpecificStorageAlias<std::int64_t> gI64(globalI64);
  for (std::int16_t i = 0; i < pando::getPlaceDims().node.id; i++) {
    auto place = pando::Place{pando::NodeIndex(i), pando::anyPod, pando::anyCore};
    auto expectPtr = pando::allocateMemory<std::int16_t>(1, place, pando::MemoryType::Main);
    EXPECT_TRUE(expectPtr.hasValue());
    pando::GlobalPtr<std::int16_t> ptr = expectPtr.value();
    auto expected = gI64.getStorageAliasAt(ptr);
    EXPECT_FALSE(expected.hasValue());
    EXPECT_EQ(expected.error(), pando::Status::OutOfBounds);
    pando::deallocateMemory(ptr, 1);
  }
}

TEST(NodeSpecificStorageAlias, SlicingSuccess) {
  pando::NodeSpecificStorageAlias<std::int64_t> gI64(globalI64);
  for (std::int16_t i = 0; i < pando::getPlaceDims().node.id; i++) {
    pando::GlobalPtr<void> ptrVoid =
        static_cast<pando::GlobalPtr<void>>(gI64.getPointerAt(pando::NodeIndex(i)));
    pando::GlobalPtr<std::int16_t> ptr = static_cast<pando::GlobalPtr<std::int16_t>>(ptrVoid);
    auto expected = gI64.getStorageAliasAt(ptr);
    ASSERT_TRUE(expected.hasValue());
    pando::NodeSpecificStorageAlias<std::int16_t> gI16AliasI64 = expected.value();
    for (std::int16_t j = 0; j < pando::getPlaceDims().node.id; j++) {
      if (i == 0) {
        *gI16AliasI64.getPointerAt(pando::NodeIndex(j)) = 0;
      } else if (i == j) {
        continue;
      } else {
        *gI16AliasI64.getPointerAt(pando::NodeIndex(j)) += i;
      }
    }
  }
  std::int16_t sum = 0;
  for (std::int16_t j = 1; j < pando::getPlaceDims().node.id; j++) {
    sum += j;
  }
  for (std::int16_t j = 0; j < pando::getPlaceDims().node.id; j++) {
    pando::GlobalPtr<std::int16_t> ptr = static_cast<pando::GlobalPtr<std::int16_t>>(
        static_cast<pando::GlobalPtr<void>>((globalI64.getPointerAt(pando::NodeIndex(j)))));
    EXPECT_EQ(*ptr, sum - j);
  }
}
