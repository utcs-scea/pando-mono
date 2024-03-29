// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "hart_context.hpp"

#include <cstring>
#include <memory>

namespace pando {

namespace {

// Guard to detect if a thread has emulated harts or not. If it does not have harts, then calling
// qthread_get_task_local() may return a random value.
// This is a write-once variable, if switched to true, it should never be reset, unless all harts
// have been shut down.
thread_local bool hasHarts = false;

} // namespace

// Sets a hart context to the qthread task local storage
Status hartContextSet(HartContext* context) noexcept {
  // set that this thread emulates a hart; it is ok for multiple qthreads to write the same value
  hasHarts = true;

  // context is stored as a pointer, since the lifetime is managed by the core
  void* p = qthread_get_tasklocal(sizeof(HartContext*));
  if (p == nullptr) {
    return Status::BadAlloc;
  }
  new (p) HartContext* {context};
  return Status::Success;
}

// Resets the hart context in the qthread task local storage
void hartContextReset() noexcept {
  if (!hasHarts) {
    return;
  }

  void* p = qthread_get_tasklocal(sizeof(HartContext*));
  if (p != nullptr) {
    std::memset(p, 0x0, sizeof(HartContext*));
  }
}

// Gets the hart context from the qthread task local storage
HartContext* hartContextGet() noexcept {
  if (!hasHarts) {
    return nullptr;
  }

  void* p = qthread_get_tasklocal(sizeof(HartContext*));
  return *static_cast<HartContext**>(p);
}

// Yields to the next hart
void hartYield(HartContext& /*context*/) noexcept {
  // TODO(ypapadop-amd) keep the context here to enforce specific scheduling
  // order
  qthread_yield();
}

// Yields to the next hart
void hartYield() noexcept {
  auto context = hartContextGet();
  if (context != nullptr) {
    qthread_yield();
  }
}

} // namespace pando
