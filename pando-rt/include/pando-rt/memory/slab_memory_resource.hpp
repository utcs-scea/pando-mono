// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_MEMORY_SLAB_MEMORY_RESOURCE_HPP_
#define PANDO_RT_MEMORY_SLAB_MEMORY_RESOURCE_HPP_

#include <cstddef>
#include <cstdint>
#include <limits>

#include "../stdlib.hpp"
#include "../sync/atomic.hpp"
#include "common_memory_resource.hpp"
#include "global_ptr.hpp"

namespace pando {

/**
 * @brief A fixed-size slab memory resource.
 *
 * The memory resource consists from a header followed by a user-addressable region. The
 * ith bit in the header field indicates the status of the ith slab; where a set bit indicates a
 * used slab.
 *
 * @ingroup ROOT
 */
template <std::uint64_t slabSize = 32>
class SlabMemoryResource {
  using BitmapType = std::uint64_t;
  using InitializationStateValueType = std::uint32_t;
  // `slabSize` must be a multiple of the bitmap size
  static_assert((slabSize % sizeof(BitmapType)) == 0);

  enum class InitializationState : std::uint32_t {
    Initialized = 0,
    InProgress = 1,
    Uninitialized = 2
  };

  // `slabSize` must be large enough to store initialization state control values
  static_assert(slabSize >= sizeof(InitializationStateValueType));

  GlobalPtr<BitmapType> m_buffer; // Pointer to the managed buffer. The pointer address is
                                  // an invariant and points to the start of the managed memory
                                  // by the resource.
  std::uint64_t m_numUserSlabs;   // Number of slabs. A constant number
                                  // that lives in register space.
  std::size_t m_capacity; // Capacity in bytes that is multiple of the `slabSize`. A constant
                          // number that lives in register space.
  std::uint64_t m_numBitmapSlabs; // Number of slabs required to store the bitmaps. A constant
                                  // number that lives in register space.
  std::uint64_t m_numBitmaps;     // Number of the bitmaps. A constant
                                  // number that lives in register space.

  GlobalPtr<InitializationStateValueType> m_initializationState; // Global state
                                                                 // describing if
                                                                 // the slab resource
                                                                 //  is initialized

  static constexpr auto bitmapBits =
      std::numeric_limits<BitmapType>::digits;       // Bits in a `BitmapType`
  static constexpr auto slabsPerBitmap = bitmapBits; // The number of slabs managed by one bitmap
  static constexpr auto fullBitmap =
      std::numeric_limits<BitmapType>::max();  // Value of a fully-used bitmap
  static constexpr BitmapType emptyBitmap = 0; // Value of a an empty bitmap

  static constexpr std::uint64_t numLockSlabs = 1; // One slab is used for initialization lock

  /**
   * @brief Convert @p InitializationState to the underlying integral type.
   */
  constexpr friend auto operator+(InitializationState initializationState) noexcept {
    return static_cast<std::underlying_type_t<InitializationState>>(initializationState);
  }

public:
  static constexpr auto resourceSlabSize = slabSize;

  /**
   * @brief Construct a new Slab Memory Resource object
   *
   * @param bufferStart Start of the buffer that the resource should use.
   * @param bufferSize The buffer size in bytes
   */
  SlabMemoryResource(GlobalPtr<std::byte> bufferStart, std::size_t bufferSize) {
    const auto requiredAlignment = slabSize;
    const auto pointerSatisfyAlignment =
        (globalPtrReinterpretCast<std::uintptr_t>(bufferStart) % requiredAlignment) == 0;
    if (!pointerSatisfyAlignment) {
      PANDO_ABORT("SlabMemoryResource must be aligned to slabSize boundary");
    }

    std::uint64_t totalNumSlabs = bufferSize / slabSize;
    // We must have at least `numLockSlabs` + 2 slabs
    if (totalNumSlabs < (numLockSlabs + 2)) {
      PANDO_ABORT("Insufficient number of slabs");
    }

    constexpr auto bitmapsPerSlab = slabSize / sizeof(BitmapType);

    // Reserve the first slab for initialization flag and lock
    totalNumSlabs = totalNumSlabs - numLockSlabs;
    m_buffer = globalPtrReinterpretCast<GlobalPtr<BitmapType>>((bufferStart));
    m_initializationState = globalPtrReinterpretCast<GlobalPtr<InitializationStateValueType>>(
        globalPtrReinterpretCast<GlobalPtr<std::byte>>(m_buffer));

    atomicStore(m_initializationState, +InitializationState::Uninitialized,
                std::memory_order_release);
    const auto lockSlabBytes = (slabSize * numLockSlabs);
    m_buffer += lockSlabBytes / sizeof(BitmapType);
    bufferSize -= lockSlabBytes;

    // Solve for the number of user slabs given that the sum of user slabs and bitmap slabs must
    // be equal to total number of slabs
    m_numUserSlabs =
        (totalNumSlabs * bitmapBits * bitmapsPerSlab) / (bitmapBits * bitmapsPerSlab + 1);

    m_numBitmapSlabs = totalNumSlabs - m_numUserSlabs;
    std::uint64_t headerBytes = m_numBitmapSlabs * slabSize;

    m_capacity = bufferSize - headerBytes;
    m_numBitmaps = nextMultipleOf(m_numUserSlabs, bitmapBits);
  }

  /**
   * @brief Allocates `bytes`
   *
   * @note Allocate does not respect alignment requests
   *
   * @param bytes The requested number of bytes
   * @return A pointer to the allocated memory or `nullptr` if the allocation failed.
   */
  [[nodiscard]] GlobalPtr<void> allocate(std::size_t bytes,
                                         std::size_t = alignof(std::max_align_t)) {
    // Lazily initialize the resource on first allocate call
    if (isUninitialized()) {
      initializeControlSlabs();
    }
    if (bytes > slabSize) {
      return nullptr;
    }
    for (std::uint64_t i = 0; i < m_numBitmaps; i++) {
      // Pointer to a bitmap (one `BitmapType` bytes)
      auto bitmap(m_buffer + i);
      bool success{false};
      // Load the current bitmap to registers
      BitmapType expected = atomicLoad(bitmap, std::memory_order_relaxed);

      // Iterate until either we succeed in the allocation or the bitmap becomes full and hence we
      // need to move to the next bitmap in the header field
      while ((!success) && (expected != fullBitmap)) {
        // Find the first unset bit in the bitmap
        BitmapType emptySlot = __builtin_ffsll(~expected) - 1;
        // Set the bit to indicate usage and store that into the `desired` CAS result
        auto mask = BitmapType{1} << emptySlot;
        auto desired = (expected | mask);
        // Attempt swapping the `expected` value with the desired value
        success = atomicCompareExchange(bitmap, expected, desired);
        if (success) {
          // Compute the slab offset computation (after the header)
          auto slabOffset = (emptySlot + i * slabsPerBitmap) * slabSize;
          auto address = getUserRegionStart() + (slabOffset / sizeof(BitmapType));
          return static_cast<GlobalPtr<void>>(address);
        }
      }
    }
    return nullptr;
  }
  /**
   * @brief  Deallocates the storage pointed to by a pointer.
   *
   * @param p Pointer to the storage to be deallocated
   */
  void deallocate(GlobalPtr<void> p, std::size_t, std::size_t = alignof(std::max_align_t)) {
    if (p == nullptr || (!pointerIsOwned(p))) {
      return;
    }

    // Find the offset of the pointer in the user region of the resource and the slab index
    auto address = static_cast<GlobalPtr<BitmapType>>(p);
    auto offset = std::distance(getUserRegionStart(), address);
    auto slabIndex = offset * sizeof(BitmapType) / slabSize;
    // Find the bitmap index and the rank (i.e., which bit) that manages this slab
    auto [bitmapIndex, bmapRank] = std::lldiv(slabIndex, slabsPerBitmap);

    // Pointer to the byte that manages this slab
    auto bitmap(m_buffer + bitmapIndex);

    // Load the current expected bitmap
    bool success{false};
    BitmapType expected = atomicLoad(bitmap, std::memory_order_relaxed);
    // Create a mask where only the bit managing the slab is unset.
    auto mask = ~(BitmapType{1} << static_cast<BitmapType>(bmapRank));
    while (!success) {
      // Unset the bit and attempt swapping the expected bitmap with the desired one where the bit
      // managing the slab is unset.
      auto desired = (expected & mask);
      success = atomicCompareExchange(bitmap, expected, desired);
    }
  }

  /**
   * @brief Equality comparison between two slab memory resources
   *
   * @param rhs A different slab memory resource
   * @return @c true If the two resources are the same
   * @return @c false If the two resources are different
   */

  bool operator==(const SlabMemoryResource& rhs) const noexcept {
    return (m_buffer == rhs.m_buffer);
  }

  /**
   * @brief Inequality comparison between two slab memory resources
   *
   * @param rhs A different slab memory resource
   * @return @c true If the two resources are not the same
   * @return @c false If the two resources are the same
   */
  bool operator!=(const SlabMemoryResource& rhs) const noexcept {
    return !(*this == rhs);
  }
  /**
   * @brief Checks if this memory resource owns the pointer
   *
   * @param p Pointer to check ownership for
   * @return @c true If the memory resource owns the pointer
   * @return @c false If the memory resource does not own the pointer
   */
  bool pointerIsOwned(GlobalPtr<const void> p) const {
    auto pointer = static_cast<GlobalPtr<const BitmapType>>(p);
    auto bufferStart = m_buffer;
    auto bufferEnd = bufferStart + m_capacity;
    return (pointer >= bufferStart) && (pointer < bufferEnd);
  }

  /**
   * @brief Checks if the memory resource can deallocate memory
   *
   * @return @c true If the resource can free memory
   * @return @c false If the resource does not free memory i.e., all calls for `deallocate(p)` are
   * no-ops.
   */
  static constexpr bool supportsFree() {
    return true;
  }

  /**
   * @brief Query the number of user-addressable bytes managed by memory resource.
   *
   * @note The number of user-addressable bytes is less than the number of bytes the resource was
   * constructed with since the memory resource uses a number of slabs for control bits.
   *
   * @return std::size_t The number of user-addressable bytes.
   */
  std::size_t bytesCapacity() const noexcept {
    return m_capacity;
  }

private:
  /**
   * @brief Get the base pointer of the slab allocator user region
   *
   * @return A global pointer to the user region start
   */
  GlobalPtr<BitmapType> getUserRegionStart() {
    return m_buffer + (m_numBitmapSlabs * slabSize / sizeof(BitmapType));
  }

  /**
   * @brief Calculates the next multiple of `multiple` greater than or equal to `value`.
   *
   * @param value The number for which to find the next multiple.
   * @param multiple The base value for which to find the next multiple of.
   * @return The next multiple of 'multiple' greater than or equal to 'value'.
   */
  static constexpr auto nextMultipleOf(std::uint64_t value, std::uint64_t multiple) {
    return (value + multiple - 1) / multiple;
  }

  /**
   * @brief Initializes the control bits of the slabs.
   */
  void initializeControlSlabs() {
    if (atomicCompareExchange(m_initializationState, +InitializationState::Uninitialized,
                              +InitializationState::InProgress)) {
      std::uint64_t headerBytes = m_numBitmapSlabs * slabSize;
      std::uint64_t headerBitmaps = headerBytes / sizeof(BitmapType);

#if defined(PANDO_RT_USE_BACKEND_PREP) || defined(PANDO_RT_USE_BACKEND_DRVX)
      auto buffer = detail::asNativePtr(m_buffer);
#else
      auto buffer = m_buffer;
#endif

      // Set all header bits to account for extra bytes in a slab
      for (std::size_t i = 0; i < m_numBitmaps; i++) {
        buffer[i] = emptyBitmap;
      }
      for (std::size_t i = m_numBitmaps; i < headerBitmaps; i++) {
        buffer[i] = fullBitmap;
      }

      // Handle corner cases where the last byte is not fully utilized
      auto lastUsableBits = m_numUserSlabs % bitmapBits;
      if (lastUsableBits != 0) {
        auto lastBitmap = ~((BitmapType{1} << lastUsableBits) - 1);
        buffer[m_numBitmaps - 1] = lastBitmap;
      }
      atomicStore(m_initializationState, +InitializationState::Initialized,
                  std::memory_order_release);
    } else {
      // spin till initialization completes
      while (isUninitialized()) {}
    }
  }
  /**
   * @brief Check if the resource is uninitialized.
   * @return @c true if the resource is uninitialized and @c false otherwise.
   */
  bool isUninitialized() const {
    const auto initializedValue = +InitializationState::Initialized;
    return atomicLoad(m_initializationState, std::memory_order_seq_cst) != initializedValue;
  }
};

} // namespace pando

#endif // PANDO_RT_MEMORY_SLAB_MEMORY_RESOURCE_HPP_
