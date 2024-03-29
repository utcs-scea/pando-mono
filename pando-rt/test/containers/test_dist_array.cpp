// SPDX-License-Identifier: MIT
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include <cstddef>
#include <iterator>
#include <vector>

#include "pando-rt/containers/array.hpp"
#include "pando-rt/locality.hpp"
#include "pando-rt/memory/allocate_memory.hpp"

namespace pando {

/**
 * @brief Distributed array structure.
 *
 * @tparam N The size of the array
 * @tparam T Type of the objects in the array
 *
 * @note This distributed array requires O(N/num_places) storage for the elements and O(num_places)
 *       storage at the creating place.
 *
 * @todo (ypapadop-amd) If N is not evenly divisible by the number of places it is distributed,
 *       the last block still has N/numBlocks elements.
 */
template <std::uint64_t N, typename T>
class DistArray {
  static_assert(std::is_trivially_destructible_v<T>, "Only supports trivially destructible types");

  Array<GlobalPtr<T>> m_blocks;

public:
  DistArray() noexcept = default;

  DistArray(const DistArray&) = delete;
  DistArray(DistArray&&) = delete;

  ~DistArray() {
    deallocateBlocks();
  }

  DistArray& operator=(const DistArray&) = delete;
  DistArray& operator=(DistArray&&) = delete;

private:
  /**
   * @brief Returns the block size.
   */
  std::uint64_t getBlockSize() const noexcept {
    return (N + m_blocks.size() - 1) / m_blocks.size();
  }

  /**
   * @brief Deallocates all blocks.
   */
  void deallocateBlocks() {
    const auto blockSize = getBlockSize();
    for (std::uint64_t j = 0; j < m_blocks.size(); ++j) {
      GlobalPtr<T> block = m_blocks[j];
      deallocateMemory(block, blockSize);
    }
    m_blocks.deinitialize();
  }

public:
  /**
   * @brief initialize a distributed array
   *
   * @tparam It the type of iterator over places
   *
   * @param[in] beg the starting iterator over places to allocate buffers
   * @param[in] end the ending iterator over places to allocate buffers
   */
  template <typename It>
  [[nodiscard]] Status initialize(It beg, It end) {
    const auto numBlocks = std::distance(beg, end);

    // allocate outer vector
    if (auto status = m_blocks.initialize(numBlocks); status != Status::Success) {
      return status;
    }

    // allocate inner vectors
    // Bulk allocation semantics
    const auto blockSize = getBlockSize();
    std::uint64_t i = 0;
    for (auto cur = beg; cur != end; ++cur, ++i) {
      PtrFuture<T> notify;
      EXPECT_EQ(notify.init(&m_blocks[i]), Status::Success);
      EXPECT_EQ(allocateMemory<T>(notify.getPromise(), blockSize, *cur, MemoryType::Main),
                Status::Success);
    }
    EXPECT_EQ(i, m_blocks.size());

    // Wait for all allocations to happen (Not all Tasks to Finish)
    bool failures = false;
    for (std::uint64_t j = 0u; j < m_blocks.size(); ++j) {
      PtrFuture<T> notify;
      EXPECT_EQ(notify.initNoReset(&m_blocks[j]), Status::Success);
      failures |= !notify.wait();
    }

    // Check failure and deallocate unneccesary state
    if (!failures) {
      return Status::Success;
    }

    // If there is a failure, free everything.
    deallocateBlocks();

    return Status::MemoryError;
  }

  /**
   * @brief Get a pointer to an element.
   *
   * @param[in] i the index of the array cell you want
   * @param[out] The pointer to that location
   */
  pando::GlobalPtr<T> get(std::uint64_t i) {
    if (i >= N) {
      return nullptr;
    }
    const auto index = i % getBlockSize();
    const auto blockId = i / getBlockSize();
    GlobalPtr<T> blockPtr = m_blocks[blockId];
    return &(blockPtr[index]);
  }
};

} // namespace pando

TEST(DistArray, TwoNodes) {
  constexpr std::uint64_t Size = 10;

  auto vec = std::vector<pando::Place>(2, pando::getCurrentPlace());

  pando::DistArray<Size, uint64_t> array;
  EXPECT_EQ(array.initialize(vec.begin(), vec.end()), pando::Status::Success);

  // initialize elements
  for (uint64_t i = 0; i < Size; i++) {
    *(array.get(i)) = i;
  }

  // access elements
  for (std::uint64_t i = 0; i < Size; i++) {
    EXPECT_EQ(*(array.get(i)), i);
  }
}

TEST(DistArray, AllNodes) {
  constexpr std::uint64_t Size = 2520;

  const auto nodes = pando::getPlaceDims().node.id;
  auto vec = std::vector<pando::Place>(nodes);
  for (std::int16_t i = 0; i < nodes; i++) {
    vec[i] = pando::Place{pando::NodeIndex{i}, pando::anyPod, pando::anyCore};
  }

  pando::DistArray<Size, std::uint64_t> array;
  EXPECT_EQ(array.initialize(vec.begin(), vec.end()), pando::Status::Success);

  // initialize elements
  for (std::uint64_t i = 0; i < Size; i++) {
    *(array.get(i)) = i;
  }

  // access elements
  for (std::uint64_t i = 0; i < Size; i++) {
    EXPECT_EQ(*(array.get(i)), i);
  }
}
