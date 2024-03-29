// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

#include "pando-rt/memory/global_ptr.hpp"

#include "pando-rt/execution/execute_on.hpp"
#include "pando-rt/memory_resource.hpp"
#include "pando-rt/sync/notification.hpp"

#include "../common.hpp"

namespace {

// Function that stores t to a global pointer
template <typename T>
void doStore(const T& t, pando::GlobalPtr<T> ptr) {
  *ptr = t;
}

// Function that loads a value from a global pointer and compares it to t
template <typename T>
void doLoad(const T& t, pando::GlobalPtr<const T> ptr, pando::Notification::HandleType handle) {
  EXPECT_EQ(*ptr, t);
  handle.notify();
}

constexpr std::string_view memoryTypeToStr(pando::MemoryType memoryType) noexcept {
  switch (memoryType) {
    case pando::MemoryType::L1SP:
      return "L1SP";
    case pando::MemoryType::L2SP:
      return "L2SP";
    case pando::MemoryType::Main:
      return "Main";
    default:
      return "unknown";
  }
}

inline std::string_view placeToStr(pando::Place place) {
  // current execution assumes that all work starts from the CP of node 0
  if (place.node.id == 0) {
    return "intranode";
  } else {
    return "internode";
  }
}

class StoreLoadOnCP : public ::testing::TestWithParam<pando::MemoryType> {};

class StoreTestL1SP : public ::testing::TestWithParam<pando::Place> {};

class StoreTest : public ::testing::TestWithParam<std::tuple<pando::MemoryType, pando::Place>> {};

class LoadTestL1SP : public ::testing::TestWithParam<pando::Place> {};

class LoadTest : public ::testing::TestWithParam<std::tuple<pando::MemoryType, pando::Place>> {};

} // namespace

TEST(GlobalPtr, CastToVoid) {
  using ValueType = std::int32_t;

  {
    pando::GlobalPtr<ValueType> ptr = nullptr;

    // cast from typed global pointer to void
    pando::GlobalPtr<void> voidPtr = ptr;
    EXPECT_EQ(ptr, voidPtr);

    // cast from void global pointer to typed
    pando::GlobalPtr<ValueType> ptr2 = static_cast<pando::GlobalPtr<ValueType>>(voidPtr);
    EXPECT_EQ(ptr, ptr2);
  }

  {
    pando::GlobalPtr<const ValueType> ptr = nullptr;

    // cast from typed global pointer to void
    pando::GlobalPtr<const void> voidPtr = ptr;
    EXPECT_EQ(ptr, voidPtr);

    // cast from void global pointer to typed
    pando::GlobalPtr<const ValueType> ptr2 =
        static_cast<pando::GlobalPtr<const ValueType>>(voidPtr);
    EXPECT_EQ(ptr, ptr2);
  }
}

TEST(GlobalPtr, UpcastDowncast) {
  const pando::GlobalPtr<Derived> ptrDerived = nullptr;

  // upcast
  const pando::GlobalPtr<Base> ptrBase = ptrDerived;
  EXPECT_EQ(ptrBase, ptrDerived);

  // downcast
  const pando::GlobalPtr<Derived> ptrDerived2 = static_cast<pando::GlobalPtr<Derived>>(ptrBase);
  EXPECT_EQ(ptrDerived2, ptrDerived);
}

TEST(GlobalPtr, CastToConst) {
  {
    pando::GlobalPtr<std::uint64_t> ptr = nullptr;

    // cast from non-const to const
    pando::GlobalPtr<const std::uint64_t> constPtr = ptr;
    EXPECT_EQ(ptr, constPtr);
  }

  {
    pando::GlobalPtr<void> ptr = nullptr;

    // cast from non-const to const
    pando::GlobalPtr<const void> constPtr = ptr;
    EXPECT_EQ(ptr, constPtr);
  }
}

TEST(GlobalPtr, ReinterpetCast) {
  constexpr std::uintptr_t ptrValue = 0x1234;

  // integral-to-pointer
  auto ptr = pando::globalPtrReinterpretCast<pando::GlobalPtr<std::uint64_t>>(ptrValue);
  static_assert(std::is_same_v<decltype(ptr), pando::GlobalPtr<std::uint64_t>>);

  // pointer-to-integral
  auto value = pando::globalPtrReinterpretCast<std::uintptr_t>(ptr);
  EXPECT_EQ(value, ptrValue);

  // pointer-to-pointer
  auto voidPtr = pando::globalPtrReinterpretCast<pando::GlobalPtr<void>>(ptr);
  static_assert(std::is_same_v<decltype(voidPtr), pando::GlobalPtr<void>>);
  EXPECT_EQ(static_cast<pando::GlobalPtr<void>>(ptr), voidPtr);

  auto bytePtr = pando::globalPtrReinterpretCast<pando::GlobalPtr<std::byte>>(ptr);
  static_assert(std::is_same_v<decltype(bytePtr), pando::GlobalPtr<std::byte>>);
  EXPECT_EQ(static_cast<pando::GlobalPtr<void>>(bytePtr), voidPtr);
}

TEST(GlobalPtr, UnaryArithmetic) {
  using ValueType = std::int32_t;
  const auto memoryType = pando::MemoryType::Main;

  using VoidPtrType = pando::GlobalPtr<void>;
  using BytePtrType = pando::GlobalPtr<std::byte>;

  // allocate memory
  const pando::GlobalPtr<ValueType> ptr =
      static_cast<pando::GlobalPtr<ValueType>>(malloc(memoryType, sizeof(ValueType)));
  ASSERT_NE(ptr, nullptr);

  {
    // unary plus: +pointer
    auto ptrUnaryPlus = ptr;
    EXPECT_EQ(ptr, +ptrUnaryPlus);
    EXPECT_EQ(ptr, ptrUnaryPlus);
  }

  {
    // pre-increment
    auto ptrPreIncrement = ptr;
    EXPECT_NE(ptr, ++ptrPreIncrement);
    EXPECT_EQ(static_cast<BytePtrType>(static_cast<VoidPtrType>(ptr)) + sizeof(ValueType),
              static_cast<BytePtrType>(static_cast<VoidPtrType>(ptrPreIncrement)));
  }

  {
    // post-increment
    auto ptrPostIncrement = ptr;
    EXPECT_EQ(ptr, ptrPostIncrement++);
    EXPECT_EQ(static_cast<BytePtrType>(static_cast<VoidPtrType>(ptr)) + sizeof(ValueType),
              static_cast<BytePtrType>(static_cast<VoidPtrType>(ptrPostIncrement)));
  }

  {
    // pre-decrement
    auto ptrPreDecrement = ptr;
    EXPECT_NE(ptr, --ptrPreDecrement);
    EXPECT_EQ(static_cast<BytePtrType>(static_cast<VoidPtrType>(ptr)) - sizeof(ValueType),
              static_cast<BytePtrType>(static_cast<VoidPtrType>(ptrPreDecrement)));
  }

  {
    // post-decrement
    auto ptrPostDecrement = ptr;
    EXPECT_EQ(ptr, ptrPostDecrement--);
    EXPECT_EQ(static_cast<BytePtrType>(static_cast<VoidPtrType>(ptr)) - sizeof(ValueType),
              static_cast<BytePtrType>(static_cast<VoidPtrType>(ptrPostDecrement)));
  }

  // free memory
  free(ptr, sizeof(ValueType));
}

TEST(GlobalPtr, BinaryArithmetic) {
  using ValueType = std::int32_t;

  // allocate memory
  const auto memoryType = pando::MemoryType::Main;

  const pando::GlobalPtr<ValueType> ptrArray =
      static_cast<pando::GlobalPtr<ValueType>>(malloc(memoryType, 2 * sizeof(ValueType)));
  ASSERT_NE(ptrArray, nullptr);

  auto ptrA = &(ptrArray[0]);
  EXPECT_NE(ptrA, nullptr);

  auto ptrB = &(ptrArray[1]);
  EXPECT_NE(ptrB, nullptr);

  // addition
  EXPECT_EQ(ptrA + 1, ptrB);
  EXPECT_EQ(1 + ptrA, ptrB);

  // subtraction
  EXPECT_EQ(ptrA, ptrB - 1);

  // chaining
  EXPECT_EQ(ptrA + 10 - 10, ptrA);

  // difference
  EXPECT_EQ(ptrB - ptrA, 1);

  free(ptrArray, 2 * sizeof(ValueType));
}

TEST(GlobalPtr, Dereference) {
  using ValueType = std::int32_t;

  // allocate memory
  const auto memoryType = pando::MemoryType::Main;
  const pando::GlobalPtr<ValueType> ptr =
      static_cast<pando::GlobalPtr<ValueType>>(malloc(memoryType, sizeof(ValueType)));
  ASSERT_NE(ptr, nullptr);

  EXPECT_EQ(ptr, &(*ptr));

  free(ptr, sizeof(ValueType));
}

TEST(GlobalPtr, Subscript) {
  using ValueType = std::int32_t;

  // allocate memory
  const auto memoryType = pando::MemoryType::Main;
  const pando::GlobalPtr<ValueType> ptr =
      static_cast<pando::GlobalPtr<ValueType>>(malloc(memoryType, sizeof(ValueType)));
  ASSERT_NE(ptr, nullptr);

  EXPECT_EQ(&(ptr[4]), ptr + 4);

  free(ptr, sizeof(ValueType));
}

TEST(GlobalPtr, LocalityOf) {
  using ValueType = std::int32_t;

  {
    auto mainMemF = [](pando::Notification::HandleType handle) {
      const auto thisPlace = pando::getCurrentPlace();
      const pando::GlobalPtr<ValueType> ptr = static_cast<pando::GlobalPtr<ValueType>>(
          malloc(pando::MemoryType::Main, sizeof(ValueType)));
      ASSERT_NE(ptr, nullptr);

      EXPECT_EQ(pando::localityOf(ptr),
                (pando::Place{thisPlace.node, pando::anyPod, pando::anyCore}));

      free(ptr, sizeof(ValueType));
      handle.notify();
    };

    pando::Notification notification;
    EXPECT_EQ(notification.init(), pando::Status::Success);

    EXPECT_EQ(pando::executeOn(
                  pando::Place{pando::NodeIndex{0}, pando::PodIndex{0, 0}, pando::CoreIndex{0, 0}},
                  +mainMemF, notification.getHandle()),
              pando::Status::Success);

    notification.wait();
  }

  {
    auto l2SPF = [](pando::Notification::HandleType handle) {
      const auto thisPlace = pando::getCurrentPlace();
      const pando::GlobalPtr<ValueType> ptr = static_cast<pando::GlobalPtr<ValueType>>(
          malloc(pando::MemoryType::L2SP, sizeof(ValueType)));
      ASSERT_NE(ptr, nullptr);

      // The L2SP is not currently partitioned between PODs.
      EXPECT_EQ(pando::localityOf(ptr).node, thisPlace.node);

      free(ptr, sizeof(ValueType));
      handle.notify();
    };

    pando::Notification notification;
    EXPECT_EQ(notification.init(), pando::Status::Success);
    EXPECT_EQ(pando::executeOn(
                  pando::Place{pando::NodeIndex{0}, pando::PodIndex{0, 0}, pando::CoreIndex{0, 0}},
                  +l2SPF, notification.getHandle()),
              pando::Status::Success);
    notification.wait();
  }

  {
    auto l1SPF = [](pando::Notification::HandleType handle) {
      const ValueType i = 0;

      const pando::GlobalPtr<const ValueType> ptr = &i;
      ASSERT_NE(ptr, nullptr);

      const auto thisPlace = pando::getCurrentPlace();
      EXPECT_EQ(pando::localityOf(ptr), thisPlace);

      handle.notify();
    };

    pando::Notification notification;
    EXPECT_EQ(notification.init(), pando::Status::Success);

    EXPECT_EQ(pando::executeOn(
                  pando::Place{pando::NodeIndex{0}, pando::PodIndex{0, 0}, pando::CoreIndex{0, 0}},
                  +l1SPF, notification.getHandle()),
              pando::Status::Success);

    notification.wait();
  }
}

TEST(GlobalPtr, L1SPTranslation) {
  using ValueType = std::int32_t;

  auto l1SPF = [](pando::Notification::HandleType handle) {
    const ValueType i = 0;

    const pando::GlobalPtr<const ValueType> ptr = &i;
    ASSERT_NE(ptr, nullptr);

    EXPECT_EQ(pando::detail::asNativePtr(ptr), &i);

    handle.notify();
  };

  pando::Notification notification;
  EXPECT_EQ(notification.init(), pando::Status::Success);

  EXPECT_EQ(pando::executeOn(
                pando::Place{pando::NodeIndex{0}, pando::PodIndex{0, 0}, pando::CoreIndex{0, 0}},
                +l1SPF, notification.getHandle()),
            pando::Status::Success);

  notification.wait();
}

TEST(GlobalPtr, TraitsRebind) {
  using FirstValueType = std::int32_t;
  using SecondValueType = std::uint64_t;
  using PointerType =
      std::pointer_traits<pando::GlobalPtr<FirstValueType>>::rebind<SecondValueType>;
  EXPECT_TRUE((std::is_same_v<PointerType, pando::GlobalPtr<SecondValueType>>));
}

#ifdef PANDO_RT_USE_BACKEND_PREP
// this test applies only to PREP because DrvX does not support converting native pointer to
// GlobalPtr
TEST(GlobalPtr, TraitsPointerTo) {
  using ValueType = std::int32_t;
  const ValueType value(42);

  // allocate memory
  const auto memoryType = pando::MemoryType::Main;
  const pando::GlobalPtr<ValueType> ptr =
      static_cast<pando::GlobalPtr<ValueType>>(malloc(memoryType, sizeof(ValueType)));
  ASSERT_NE(ptr, nullptr);

  // Extract native pointer and re-construct global pointer
  *ptr = value;
  auto nativePtr = pando::detail::asNativePtr(ptr);
  auto reconstructedPtr = std::pointer_traits<pando::GlobalPtr<ValueType>>::pointer_to(*nativePtr);

  EXPECT_EQ(ptr, reconstructedPtr);

  free(ptr, sizeof(ValueType));
}
#endif

TEST(GlobalPtr, ArrowOperator) {
  using ValueType = TriviallyCopyable;
  const TriviallyCopyable value(42);

  // allocate memory and initialize
  const auto memoryType = pando::MemoryType::Main;
  pando::GlobalPtr<ValueType> ptr =
      static_cast<pando::GlobalPtr<ValueType>>(malloc(memoryType, sizeof(ValueType)));
  ASSERT_NE(ptr, nullptr);
  *ptr = ValueType{};

  // member access
  ptr->i32 = value.i32;
  ptr->c = value.c;
  ptr->b = value.b;
  ptr->i64 = value.i64;
  ptr->u16 = value.u16;

  EXPECT_EQ(*ptr, value);

  free(ptr, sizeof(ValueType));
}

TEST(GlobalPtr, ArrowOperatorWithGlobalPtr) {
  struct S {
    pando::GlobalPtr<std::int32_t> i32Ptr;
  };

  using ValueType = S;

  // allocate memory and initialize
  const auto memoryType = pando::MemoryType::Main;
  pando::GlobalPtr<ValueType> ptr =
      static_cast<pando::GlobalPtr<ValueType>>(malloc(memoryType, sizeof(ValueType)));
  ASSERT_NE(ptr, nullptr);

  ptr->i32Ptr =
      static_cast<pando::GlobalPtr<std::int32_t>>(malloc(memoryType, sizeof(std::int32_t)));
  ASSERT_NE(ptr->i32Ptr, nullptr);

  // member access
  *(ptr->i32Ptr) = 10;
  EXPECT_EQ(*ptr->i32Ptr, 10);

  free(ptr->i32Ptr, sizeof(std::int32_t));
  free(ptr, sizeof(ValueType));
}

TEST_P(StoreLoadOnCP, Int32) {
  using ValueType = std::int32_t;
  const auto memoryType = GetParam();

  const ValueType value(42);

  // allocate memory
  const pando::GlobalPtr<ValueType> ptr =
      static_cast<pando::GlobalPtr<ValueType>>(malloc(memoryType, sizeof(ValueType)));
  ASSERT_NE(ptr, nullptr);

  // store and load
  *ptr = ValueType{};
  ASSERT_NE(*ptr, value);
  *ptr = value;
  ASSERT_EQ(*ptr, value);

  free(ptr, sizeof(ValueType));
}

TEST_P(StoreTestL1SP, Int32) {
  using ValueType = std::int32_t;
  const auto place = GetParam();

  auto f = [](pando::Place place, pando::Notification::HandleType handle) {
    const ValueType value(42);

    ValueType v;

    // create pointer
    pando::GlobalPtr<ValueType> ptr = &v;
    ASSERT_NE(ptr, nullptr);

    // initialize
    *ptr = ValueType{};
    EXPECT_NE(*ptr, value);

    // do remote store
    EXPECT_EQ(pando::executeOn(place, &doStore<ValueType>, value, ptr), pando::Status::Success);

    // wait to do the store
    pando::waitUntil([&] {
      return (*ptr == value);
    });

    EXPECT_EQ(*ptr, value);

    handle.notify();
  };

  pando::Notification notification;
  EXPECT_EQ(notification.init(), pando::Status::Success);

  EXPECT_EQ(pando::executeOn(pando::Place{}, +f, place, notification.getHandle()),
            pando::Status::Success);

  notification.wait();
}

TEST_P(StoreTest, Int32) {
  using ValueType = std::int32_t;
  const auto memoryType = std::get<0>(GetParam());
  const auto place = std::get<1>(GetParam());

  const ValueType value(42);

  // allocate memory
  const pando::GlobalPtr<ValueType> ptr =
      static_cast<pando::GlobalPtr<ValueType>>(malloc(memoryType, sizeof(ValueType)));
  ASSERT_NE(ptr, nullptr);

  // initialize
  *ptr = ValueType{};
  EXPECT_NE(*ptr, value);

  // do remote store
  EXPECT_EQ(pando::executeOn(place, &doStore<ValueType>, value, ptr), pando::Status::Success);

  // wait to do the store
  pando::waitUntil([&] {
    return (*ptr == value);
  });

  EXPECT_EQ(*ptr, value);

  free(ptr, sizeof(ValueType));
}

TEST_P(StoreTest, LargeObject) {
  using ValueType = LargeFunctionObject;

  const auto memoryType = std::get<0>(GetParam());
  const auto place = std::get<1>(GetParam());

  const ValueType value(42);

  // allocate memory
  const pando::GlobalPtr<ValueType> ptr =
      static_cast<pando::GlobalPtr<ValueType>>(malloc(memoryType, sizeof(ValueType)));
  ASSERT_NE(ptr, nullptr);

  // initialize
  *ptr = ValueType{};
  EXPECT_NE(*ptr, value);

  // do remote store
  EXPECT_EQ(pando::executeOn(place, &doStore<ValueType>, value, ptr), pando::Status::Success);

  // timed wait to do the store
  pando::waitUntil([&] {
    return (*ptr == value);
  });

  EXPECT_EQ(*ptr, value);

  free(ptr, sizeof(ValueType));
}

TEST_P(LoadTestL1SP, Int32) {
  using ValueType = std::int32_t;
  const auto place = GetParam();

  auto f = [](pando::Place place, pando::Notification::HandleType handle) {
    const ValueType value(42);

    // create pointer
    pando::GlobalPtr<const ValueType> ptr = &value;
    ASSERT_NE(ptr, nullptr);

    // do remote load
    pando::Notification loadNotification;
    EXPECT_EQ(loadNotification.init(), pando::Status::Success);

    EXPECT_EQ(pando::executeOn(place, &doLoad<ValueType>, value, ptr, loadNotification.getHandle()),
              pando::Status::Success);

    // wait for the load to finish
    loadNotification.wait();

    handle.notify();
  };

  pando::Notification notification;
  EXPECT_EQ(notification.init(), pando::Status::Success);

  EXPECT_EQ(pando::executeOn(pando::Place{}, +f, place, notification.getHandle()),
            pando::Status::Success);

  notification.wait();
}

TEST_P(LoadTest, Int32) {
  using ValueType = std::int32_t;
  const auto memoryType = std::get<0>(GetParam());
  const auto place = std::get<1>(GetParam());

  const ValueType value(42);

  // allocate and initialize memory
  const pando::GlobalPtr<ValueType> ptr =
      static_cast<pando::GlobalPtr<ValueType>>(malloc(memoryType, sizeof(ValueType)));
  ASSERT_NE(ptr, nullptr);

  *ptr = value;

  // do remote load
  pando::Notification notification;
  EXPECT_EQ(notification.init(), pando::Status::Success);
  EXPECT_EQ(pando::executeOn(place, &doLoad<ValueType>, value, ptr, notification.getHandle()),
            pando::Status::Success);

  // wait for the load to finish
  notification.wait();

  free(ptr, sizeof(ValueType));
}

TEST_P(LoadTest, LargeObject) {
  using ValueType = LargeFunctionObject;
  const auto memoryType = std::get<0>(GetParam());
  const auto place = std::get<1>(GetParam());

  const ValueType value(42);

  // allocate and initialize memory
  const pando::GlobalPtr<ValueType> ptr =
      static_cast<pando::GlobalPtr<ValueType>>(malloc(memoryType, sizeof(ValueType)));
  ASSERT_NE(ptr, nullptr);

  *ptr = value;

  // do remote load
  pando::Notification notification;
  EXPECT_EQ(notification.init(), pando::Status::Success);
  EXPECT_EQ(pando::executeOn(place, &doLoad<ValueType>, value, ptr, notification.getHandle()),
            pando::Status::Success);

  // wait for the load to finish
  notification.wait();

  free(ptr, sizeof(ValueType));
}

constexpr pando::MemoryType memoryTypes[] = {pando::MemoryType::L2SP, pando::MemoryType::Main};

constexpr pando::Place places[] = {
    // memory access within node 0
    pando::Place{pando::NodeIndex{0}, pando::PodIndex{0, 0}, pando::CoreIndex{0, 0}},
    // memory access between nodes 0 and 1
    pando::Place{pando::NodeIndex{1}, pando::PodIndex{0, 0}, pando::CoreIndex{0, 0}},
};

INSTANTIATE_TEST_SUITE_P(GlobalPtr, StoreLoadOnCP, ::testing::ValuesIn(memoryTypes),
                         [](const ::testing::TestParamInfo<StoreLoadOnCP::ParamType>& info) {
                           const auto& memoryType = info.param;
                           return std::string(memoryTypeToStr(memoryType));
                         });

INSTANTIATE_TEST_SUITE_P(GlobalPtr, StoreTestL1SP, ::testing::ValuesIn(places),
                         [](const ::testing::TestParamInfo<StoreTestL1SP::ParamType>& info) {
                           const auto& place = info.param;
                           return std::string(placeToStr(place));
                         });

INSTANTIATE_TEST_SUITE_P(GlobalPtr, StoreTest,
                         ::testing::Combine(::testing::ValuesIn(memoryTypes),
                                            ::testing::ValuesIn(places)),
                         [](const ::testing::TestParamInfo<StoreTest::ParamType>& info) {
                           const auto& memoryType = std::get<0>(info.param);
                           const auto& place = std::get<1>(info.param);

                           return std::string(memoryTypeToStr(memoryType)) + "_" +
                                  std::string(placeToStr(place));
                         });

INSTANTIATE_TEST_SUITE_P(GlobalPtr, LoadTestL1SP, ::testing::ValuesIn(places),
                         [](const ::testing::TestParamInfo<LoadTestL1SP::ParamType>& info) {
                           const auto& place = info.param;
                           return std::string(placeToStr(place));
                         });

INSTANTIATE_TEST_SUITE_P(GlobalPtr, LoadTest,
                         ::testing::Combine(::testing::ValuesIn(memoryTypes),
                                            ::testing::ValuesIn(places)),
                         [](const ::testing::TestParamInfo<LoadTest::ParamType>& info) {
                           const auto& memoryType = std::get<0>(info.param);
                           const auto& place = std::get<1>(info.param);

                           return std::string(memoryTypeToStr(memoryType)) + "_" +
                                  std::string(placeToStr(place));
                         });
