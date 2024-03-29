// SPDX-License-Identifier: MIT
/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include <pando-rt/sync/future.hpp>

#include "../common.hpp"

using pando::GlobalPtr, pando::PtrFuture;

TEST(Sync, PtrFutureInit) {
  auto ptrPtr = static_cast<pando::GlobalPtr<pando::GlobalPtr<std::uint64_t>>>(
      malloc(pando::MemoryType::Main, sizeof(GlobalPtr<std::uint64_t>)));

  PtrFuture<std::uint64_t> future;
  EXPECT_EQ(future.init(ptrPtr), pando::Status::Success);
  EXPECT_EQ(future.init(ptrPtr), pando::Status::AlreadyInit);

  free(ptrPtr, sizeof(GlobalPtr<std::uint64_t>));
}

TEST(Sync, PtrFutureFailInit) {
  PtrFuture<std::uint64_t> future;
  EXPECT_EQ(future.init(nullptr), pando::Status::InvalidValue);
  EXPECT_EQ(future.initNoReset(nullptr), pando::Status::InvalidValue);
}

TEST(Sync, PtrFutureWait) {
  auto ptrPtr = static_cast<pando::GlobalPtr<pando::GlobalPtr<std::uint64_t>>>(
      malloc(pando::MemoryType::Main, sizeof(GlobalPtr<std::uint64_t>)));
  pando::PtrFuture<std::uint64_t> future;
  EXPECT_EQ(future.init(ptrPtr), pando::Status::Success);
  auto promise = future.getPromise();
  auto ptr = static_cast<pando::GlobalPtr<std::uint64_t>>(
      malloc(pando::MemoryType::Main, sizeof(std::uint64_t)));
  promise.setValue(ptr);
  EXPECT_TRUE(future.wait());
  EXPECT_EQ(*ptrPtr, ptr);
  free(ptr, sizeof(std::uint64_t));
  free(ptrPtr, sizeof(GlobalPtr<std::uint64_t>));
}

TEST(Sync, PtrFutureInitNoReset) {
  auto ptrPtr = static_cast<pando::GlobalPtr<pando::GlobalPtr<std::uint64_t>>>(
      malloc(pando::MemoryType::Main, sizeof(GlobalPtr<std::uint64_t>)));
  PtrFuture<std::uint64_t> future0;
  PtrFuture<std::uint64_t> future1;
  EXPECT_EQ(future0.init(ptrPtr), pando::Status::Success);
  EXPECT_EQ(future1.initNoReset(ptrPtr), pando::Status::Success);
  auto promise = future0.getPromise();
  auto ptr = static_cast<pando::GlobalPtr<std::uint64_t>>(
      malloc(pando::MemoryType::Main, sizeof(std::uint64_t)));
  promise.setValue(ptr);
  EXPECT_TRUE(future1.wait());
  EXPECT_EQ(*ptrPtr, ptr);
  free(ptr, sizeof(std::uint64_t));
  free(ptrPtr, sizeof(GlobalPtr<std::uint64_t>));
}

TEST(Sync, PtrFutureWaitFailure) {
  auto ptrPtr = static_cast<pando::GlobalPtr<pando::GlobalPtr<std::uint64_t>>>(
      malloc(pando::MemoryType::Main, sizeof(GlobalPtr<std::uint64_t>)));
  pando::PtrFuture<std::uint64_t> future;
  EXPECT_EQ(future.init(ptrPtr), pando::Status::Success);
  auto promise = future.getPromise();
  auto ptr = static_cast<pando::GlobalPtr<std::uint64_t>>(
      malloc(pando::MemoryType::Main, sizeof(std::uint64_t)));
  promise.setFailure();
  EXPECT_FALSE(future.wait());
  free(ptr, sizeof(std::uint64_t));
  free(ptrPtr, sizeof(GlobalPtr<std::uint64_t>));
}
