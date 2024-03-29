// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "memory.hpp"

#include <cstring>
#include <memory>

#include "pando-rt/memory/address_translation.hpp"

#include "config.hpp"
#include "cores.hpp"
#include "status.hpp"

namespace pando {

namespace {

// Object that represents a memory
class MemoryChunk {
  Memory::Information m_information{};
  std::unique_ptr<std::byte[]> m_backingStore;

public:
  // Initializes the chunk with the given memory type and size and zeroes the first zeroFillBytes
  [[nodiscard]] Status initialize(MemoryType memoryType, std::size_t size,
                                  std::size_t zeroFillBytes) noexcept {
    try {
      m_backingStore.reset(new std::byte[size]);
      m_information = {memoryType, m_backingStore.get(), size};
    } catch (const std::bad_alloc&) {
      return Status::BadAlloc;
    }
    std::memset(m_backingStore.get(), 0x0, zeroFillBytes);
    return Status::Success;
  }

  void reset() {
    m_backingStore.reset();
    m_information = {};
  }

  // Returns if the pointer points to an object in this memory
  constexpr bool ownsAddress(const void* nativePtr) const noexcept {
    return (nativePtr >= m_information.baseAddress) &&
           (nativePtr < (m_information.baseAddress + m_information.byteCount));
  }

  // Returns the information associated with this memory
  constexpr const Memory::Information& getInformation() const noexcept {
    return m_information;
  }

  // Returns the base address of this memory
  constexpr std::byte* getBaseAddress() const noexcept {
    return m_information.baseAddress;
  }
};

// L2SP memory
// TODO(ypapadop): this memory needs to be logically partitioned among pods
MemoryChunk l2SP;

// main memory
MemoryChunk main;

} // namespace

Status Memory::initialize(std::size_t l2SPZeroFillBytes, std::size_t mainZeroFillBytes) {
  const auto& config = Config::getCurrentConfig();

  // initialize L2SP
  if (auto status = l2SP.initialize(MemoryType::L2SP, config.memory.l2SPPod, l2SPZeroFillBytes);
      status != Status::Success) {
    return status;
  }

  // initialize main memory
  if (auto status = main.initialize(MemoryType::Main, config.memory.mainNode, mainZeroFillBytes);
      status != Status::Success) {
    return status;
  }

  return Status::Success;
}

void Memory::finalize() {
  l2SP.reset();
  main.reset();
}

const Memory::Information& Memory::getInformation(MemoryType memoryType) noexcept {
  switch (memoryType) {
    case MemoryType::L2SP:
      return l2SP.getInformation();

    case MemoryType::Main:
      return main.getInformation();

    case MemoryType::L1SP: // L1SP is not a contiguous memory, it's handled separately
    case MemoryType::Unknown:
    default: {
      static const Memory::Information empty{MemoryType::Unknown, nullptr, 0};
      return empty;
    }
  }
}

const Memory::Information& Memory::findInformation(const void* nativePtr) noexcept {
  // check if it's within bounds of L2SP
  if (const auto& memory = l2SP; memory.ownsAddress(nativePtr)) {
    return memory.getInformation();
  }

  // check if it's within bounds of main memory
  if (const auto& memory = main; memory.ownsAddress(nativePtr)) {
    return memory.getInformation();
  }

  // nativePtr may be L1SP or not from any known memory
  static const Memory::Information empty{MemoryType::Unknown, nullptr, 0};
  return empty;
}

void* Memory::getNativeAddress(GlobalAddress addr) noexcept {
  const auto memoryType = extractMemoryType(addr);

  switch (memoryType) {
    case MemoryType::L1SP: {
      const auto podIdx = extractPodIndex(addr);
      const auto coreIdx = extractCoreIndex(addr);
      const std::size_t offset = readBits(addr, addressMap.L1SP.offset);
      return Cores::getL1SPLocalAdddress(podIdx, coreIdx, offset);
    }

    case MemoryType::L2SP: {
      const auto baseAddress = l2SP.getBaseAddress();
      const std::size_t offset = readBits(addr, addressMap.L2SP.offset);
      return baseAddress + offset;
    }

    case MemoryType::Main: {
      const auto baseAddress = main.getBaseAddress();
      const std::size_t offset = readBits(addr, addressMap.main.offset);
      return baseAddress + offset;
    }

    case MemoryType::Unknown:
    default:
      return nullptr;
  }
}

} // namespace pando
