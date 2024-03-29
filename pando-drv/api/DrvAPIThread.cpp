// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include "DrvAPIThread.hpp"
#include "DrvAPIAddressMap.hpp"
#include "DrvAPIAddressToNative.hpp"
#include "DrvAPIAllocator.hpp"
#include "DrvAPIGlobal.hpp"
#include "DrvAPIMain.hpp"
#include <iostream>
#include <sstream>
using namespace DrvAPI;

/**
 * class that allocates a stack buffer for the coroutine
 * this allocates memory from the modeled memHierarcy memories
 */
struct modeled_memory_stack_allocator {
public:
  modeled_memory_stack_allocator() = default;
  modeled_memory_stack_allocator(DrvAPIThread* thread) : thread_(thread) {}

  boost::context::stack_context allocate() {
    boost::context::stack_context sctx;
    // 1. determine end of l1sp statics
    auto& l1sp_statics = DrvAPI::DrvAPISection::GetSection(DrvAPIMemoryL1SP);

    DrvAPI::DrvAPIAddress l1sp_static_base =
        l1sp_statics.getBase(thread_->pxnId(), thread_->podId(), thread_->coreId());

    DrvAPI::DrvAPIAddress l1sp_static_end_local = l1sp_static_base + l1sp_statics.getSize();

    DrvAPI::DrvAPIAddress l1sp_static_end =
        DrvAPI::toGlobalAddress(l1sp_static_end_local, thread_->pxnId(), thread_->podId(),
                                coreYFromId(thread_->coreId()), coreXFromId(thread_->coreId()));

    // 2. determine the total available stack size and divide amongst theads
    // this is just the rest of l1sp
    uint64_t stack_bytes = coreL1SPSize() - l1sp_statics.getSize();
    uint64_t stack_words = stack_bytes / sizeof(uint64_t);
    uint64_t thread_stack_words = stack_words / thread_->coreThreads();
    uint64_t thread_stack_bytes = thread_stack_words * sizeof(uint64_t);

    // 3. calculate the top of the stack for this thread
    DrvAPI::DrvAPIAddress stack_top =
        l1sp_static_end + (thread_->threadId() + 1) * thread_stack_bytes - sizeof(uint64_t);

    // 4. get the native stack pointer using toNative()
    size_t _;
    thread_->addressToNative(stack_top, &sctx.sp, &_);
    sctx.size = thread_stack_bytes - sizeof(uint64_t);
    return sctx;
  }
  void deallocate(boost::context::stack_context& sctx) {
    sctx.sp = nullptr;
    sctx.size = 0;
  }

  DrvAPIThread* thread_ = nullptr;
};

DrvAPIThread::DrvAPIThread()
    : thread_context_(nullptr),
      state_(new DrvAPIThreadIdle),
      main_(nullptr),
      argc_(0),
      argv_(nullptr) {}

void DrvAPIThread::start() {
  auto coro_function = [this](coro_t::push_type& sink) {
    this->main_context_ = &sink;
    this->yield();
    while (true) {
      if (this->main_) {
        this->main_(argc_, argv_);
        this->main_ = nullptr;
        this->state_ = std::make_shared<DrvAPITerminate>();
      }
      this->yield();
    }
  };
  if (stack_in_modeled_memory_) {
    modeled_memory_stack_allocator allocator(this);
    thread_context_ = std::make_unique<coro_t::pull_type>(allocator, coro_function);
  } else {
    thread_context_ = std::make_unique<coro_t::pull_type>(coro_function);
  }
}

/* should only be called from the thread context */
void DrvAPIThread::yield(DrvAPIThreadState* state) {
  state_ = std::make_shared<DrvAPIThreadState>(*state);
  yield();
}

/* should only be called from the thread context */
void DrvAPIThread::yield() {
  (*main_context_)();
}

/* should only be called from the main context */
void DrvAPIThread::resume() {
  (*thread_context_)();
}

/* callable from anywhere */
void DrvAPIThread::addressToNative(DrvAPIAddress address, void** native, std::size_t* size) {
  DrvAPIAddress phys_address =
      DrvAPIVAddress::to_physical(address, pxn_id_, pod_id_, coreYFromId(core_id_),
                                  coreXFromId(core_id_))
          .encode();
  getSystem()->addressToNative(phys_address, native, size);
}

/* callable from anywhere */
void DrvAPIThread::nativeToAddress(void* native, DrvAPIAddress* address, std::size_t* size) {
  /* we are only going to support this function when using modeled memory for stack
     and we are only going to support this pointers to our own l1sp
  */
  // 1. check that we are using modeled memory for stack
  if (!stack_in_modeled_memory_) {
    std::stringstream ss;
    throw std::runtime_error(
        "DrvAPIThread::nativeToAddress() only supported when using modeled memory for stack");
  }
  // 2. get native pointer to the base of l1sp
  DrvAPIAddress l1sp_base = toGlobalAddress(DrvAPIVAddress::MyL1Base().encode(), pxnId(), podId(),
                                            coreYFromId(coreId()), coreXFromId(coreId()));
  void* l1sp_base_native = nullptr;
  size_t l1sp_base_size = 0;
  addressToNative(l1sp_base, &l1sp_base_native, &l1sp_base_size);

  // 3. check that the native pointer is within the l1sp
  uintptr_t start = reinterpret_cast<uintptr_t>(l1sp_base_native);
  uintptr_t check = reinterpret_cast<uintptr_t>(native);
  if (check < start || check >= start + l1sp_base_size) {
    std::stringstream ss;
    ss << "DrvAPIThread::nativeToAddress() native pointer " << std::hex << check
       << " is not within l1sp";
    throw std::runtime_error(ss.str());
  }

  *address = l1sp_base + (check - start);
  *size = l1sp_base_size - (check - start);
}

/**
 * @brief Set the execution tag
 *
 * @param tag
 * @return the old tag
 */
int DrvAPIThread::setTag(int tag) {
  int old_tag = tag_;
  tag_ = tag;
  return old_tag;
}

/**
 * @brief Get the execution tag
 *
 * @return the tag
 */
int DrvAPIThread::getTag() const {
  return tag_;
}

thread_local DrvAPIThread* DrvAPIThread::g_current_thread = nullptr;

DrvAPIThread* DrvAPIGetCurrentContext() {
  return DrvAPIThread::g_current_thread;
}

void DrvAPISetCurrentContext(DrvAPI::DrvAPIThread* thread) {
  DrvAPIThread::g_current_thread = thread;
}
