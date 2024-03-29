// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_TEST_COMMON_HPP_
#define PANDO_RT_TEST_COMMON_HPP_

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <random>
#include <type_traits>
#include <vector>

#include "gtest/gtest.h"

#include "pando-rt/execution/execute_on_wait.hpp"
#include "pando-rt/memory/global_ptr.hpp"
#include "pando-rt/memory_resource.hpp"

// Allocates size bytes of uninitialized storage from the memory closer to the calling place.
inline pando::GlobalPtr<void> malloc(pando::MemoryType memoryType, std::size_t size) {
  switch (memoryType) {
    case pando::MemoryType::L2SP: {
      auto place = pando::getCurrentPlace();
      if (pando::isOnCP()) {
        place.pod = pando::PodIndex{0, 0};
        place.core = pando::CoreIndex{0, 0};
      }

      auto f = +[](std::size_t size) {
        auto memoryResource = pando::getDefaultL2SPResource();
        auto ptr = memoryResource->allocate(size);
        return ptr;
      };

      auto result = pando::executeOnWait(place, f, size);
      EXPECT_TRUE(result.hasValue()) << "executeOnError(): " << pando::errorString(result.error());
      return result.value();
    }

    case pando::MemoryType::Main: {
      auto memoryResource = pando::getDefaultMainMemoryResource();
      auto ptr = memoryResource->allocate(size);
      return ptr;
    }

    default:
      EXPECT_TRUE(false) << "Incorrect memory type";
      return nullptr;
  }
}

// Deallocates the pointe allocated by malloc(). It needs to be called from the same place that
// malloc() was called.
inline void free(pando::GlobalPtr<void> ptr, std::size_t size) {
  const auto ptrPlace = pando::localityOf(ptr);

  switch (memoryTypeOf(ptr)) {
    case pando::MemoryType::L2SP: {
      auto f = +[](pando::GlobalPtr<void> ptr, std::size_t size) {
        auto memoryResource = pando::getDefaultL2SPResource();
        memoryResource->deallocate(ptr, size);
      };

      auto result = pando::executeOnWait(ptrPlace, f, ptr, size);
      EXPECT_TRUE(result.hasValue()) << "executeOnError(): " << pando::errorString(result.error());
    } break;

    case pando::MemoryType::Main: {
      EXPECT_TRUE(pando::getCurrentNode() == ptrPlace.node);
      auto memoryResource = pando::getDefaultMainMemoryResource();
      memoryResource->deallocate(ptr, size);
    } break;

    default:
      FAIL() << "Incorrect memory type";
  }
}

class EmptyClass {
public:
  constexpr bool operator==(const EmptyClass&) const noexcept {
    return true;
  }
};

enum class Enum : std::uint8_t { Value0, Value1, Value2 };

struct Aggregate {
  std::int32_t i32;
  char c;
  bool b;
  std::int64_t i64;
  std::uint16_t u16;

  bool operator==(const Aggregate& other) const noexcept {
    return i32 == other.i32 && c == other.c && b == other.b && i64 == other.i64 && u16 == other.u16;
  }
};
static_assert(std::is_trivial_v<Aggregate>);

struct TriviallyCopyable {
  std::int32_t i32{};
  char c{};
  bool b{};
  std::int64_t i64{};
  std::uint16_t u16{};

  explicit TriviallyCopyable(std::int32_t i = 1) {
    i32 = i;
    c = i + 1;
    b = c != i;
    i64 = i + 2;
    u16 = i + 3;
  }

  bool operator==(const TriviallyCopyable& other) const noexcept {
    return i32 == other.i32 && c == other.c && b == other.b && i64 == other.i64 && u16 == other.u16;
  }
};
static_assert(!std::is_trivial_v<TriviallyCopyable> &&
              std::is_trivially_copyable_v<TriviallyCopyable>);

std::int64_t fun() {
  return 42;
}

std::int64_t funNoexcept() noexcept {
  return 42;
}

void funI(std::int64_t i) {
  EXPECT_EQ(i, 42);
}

std::int64_t& funRef(std::int64_t& i) {
  return i;
}

void funIB(std::int64_t i, bool b) {
  EXPECT_EQ(i, 42);
  EXPECT_EQ(b, true);
}

struct FunctionObject {
  std::int64_t operator()() noexcept {
    return 42;
  }

  std::int64_t memFun() const {
    return 43;
  }
};

struct LargeFunctionObject {
  constexpr static std::size_t size = 1024;
  std::uint8_t data[size];

  explicit LargeFunctionObject(std::int64_t seed = 0) {
    randomize(seed);
  }

  void randomize(std::int64_t seed = 0) {
    std::linear_congruential_engine<std::uint8_t, 2u, 3u, 8u> rng(seed);
    std::generate_n(data, size, [&] {
      return rng();
    });
  }

  std::int64_t operator()() noexcept {
    return 42;
  }

  bool operator==(const LargeFunctionObject& other) const noexcept {
    return std::equal(data, data + size, other.data);
  }

  bool operator!=(const LargeFunctionObject& other) const noexcept {
    return !(*this == other);
  }
};
static_assert(std::is_trivially_copyable_v<LargeFunctionObject>);

// Object that counts moves and copies
struct CountingObject {
  std::int64_t copies{};
  std::int64_t moves{};

  CountingObject() = default;

  CountingObject(const CountingObject& other) : copies(other.copies + 1), moves(other.moves) {}
  CountingObject(CountingObject&& other) : copies(other.copies), moves(other.moves + 1) {}

  ~CountingObject() = default;

  CountingObject& operator=(const CountingObject& other) {
    if (this != &other) {
      copies = other.copies + 1;
      moves = other.moves;
    }
    return *this;
  }

  CountingObject& operator=(CountingObject&& other) {
    if (this != &other) {
      copies = other.copies;
      moves = other.moves + 1;
    }
    return *this;
  }

  template <typename Archive>
  void serialize(Archive& ar) {
    ar(copies, moves);
  }
};

constexpr inline auto createVector = [](std::size_t n = 100) {
  std::vector<std::int32_t> t(n);
  std::iota(t.begin(), t.end(), 2);
  return t;
};

struct Base {};

struct Derived : public Base {};

#endif // PANDO_RT_TEST_COMMON_HPP_
