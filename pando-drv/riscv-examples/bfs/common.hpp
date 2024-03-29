// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#pragma once
#ifdef COMMAND_PROCESSOR
#include <DrvAPI.hpp>
#else
#include <pandohammer/cpuinfo.h>
#include <pandohammer/atomic.h>
#endif
#include <cstdint>
#include <stdarg.h>
#include <string.h>
#ifdef COMMAND_PROCESSOR
static inline int threads() { return DrvAPI::numPodCores() * THREADS_PER_CORE; }
static inline int my_thread() { return DrvAPI::myThreadId(); }
#else
static inline int threads() { return numPodCores() * myCoreThreads(); }
static inline int my_thread() { return myCoreThreads()*myCoreId() + myThreadId(); }
#endif

int thread_safe_printf(const char* fmt, ...)
{
    char buf[256];
    va_list va;
    va_start(va, fmt);
    int ret = vsnprintf(buf, sizeof(buf), fmt, va);
    write(STDOUT_FILENO, buf, strlen(buf));
    va_end(va);
    return ret;
}

#ifdef COMMAND_PROCESSOR
static inline int core() { return DrvAPI::myCoreId(); }
static inline int thread_on_core() { return DrvAPI::myThreadId(); }
#else
static inline int core() { return myCoreId(); }
static inline int thread_on_core() { return myThreadId(); }
#endif

#define pr_info(fmt, ...)                                               \
    thread_safe_printf("PH: Core %d, Thread %d: " fmt                   \
                       ,core()                                          \
                       ,thread_on_core()                                \
                       ,##__VA_ARGS__)

#define DEBUG
#ifdef  DEBUG
#define pr_dbg(fmt,...)                         \
    pr_info(fmt, ##__VA_ARGS__)
#else
#define pr_dbg(fmt,...)
#endif

/////////////////////////////////////
// Utility variables and functions //
/////////////////////////////////////
/**
 * Get the number of threads running
 */
#ifdef COMMAND_PROCESSOR
#define THREADS                                 \
    (DrvAPI::myCoreThreads() * DrvAPI::numPodCores())
#else
#define THREADS                                 \
    (myCoreThreads() * numPodCores())
#endif

/**
 * wait for x cycles
 */
#ifdef COMMAND_PROCESSOR
inline void wait(int x) {
    DrvAPI::wait(x);
}
#else
inline void wait(volatile int x) {
    for(int i  = 0; i < x; i++) {
        asm volatile("nop");
    }
}
#endif

//////////////////////////////
// reference class wrappers //
//////////////////////////////
#ifdef COMMAND_PROCESSOR
#define REF_CLASS_BEGIN(type)                   \
    DRV_API_REF_CLASS_BEGIN(type)

#define REF_CLASS_DATA_MEMBER(type, member)     \
    DRV_API_REF_CLASS_DATA_MEMBER(type, member)

#define REF_CLASS_END(type)                     \
    DRV_API_REF_CLASS_END(type)
#else
#define REF_CLASS_BEGIN(type)                                   \
    class type##_ref {                                          \
    public:                                                     \
    type##_ref(type *ptr) : ptr_(ptr) {}                        \
    type##_ref() = delete;                                      \
    type##_ref(const type##_ref &other) = default;              \
    type##_ref(type##_ref &&other) = default;                   \
    type##_ref &operator=(const type##_ref &other) = default;   \
    type##_ref &operator=(type##_ref &&other) = default;        \
    ~type##_ref() = default;                                    \
    type* operator&() { return ptr_; }                          \
    type *ptr_;

#define REF_CLASS_DATA_MEMBER(type, member)             \
    decltype(std::declval<type>().member) &             \
    member() const { return ptr_->member; }

#define REF_CLASS_END(type)                     \
    };

#endif


///////////
// types //
///////////
using vertex_t = int32_t;

#ifdef COMMAND_PROCESSOR
using vertex_pointer_t = DrvAPI::DrvAPIPointer<vertex_t>;
using vertex_ref_t = DrvAPI::DrvAPIPointer<vertex_t>::value_handle;
#else
using vertex_pointer_t = vertex_t *;
using vertex_ref_t = vertex_t &;
#endif


/**
 * @brief A barrier struct
 * This struct is used to synchronize threads
 */
struct barrier_data {
    int count;
    int signal;
    int sense;
};

/**
 * creates the barrier_data_ref class
 */
REF_CLASS_BEGIN(barrier_data)
  REF_CLASS_DATA_MEMBER(barrier_data,count)
  REF_CLASS_DATA_MEMBER(barrier_data,signal)
  REF_CLASS_DATA_MEMBER(barrier_data,sense)
  void sync() {
    sync([](){});
  }
  template <typename F>
  void sync(F f) {
    int signal_ = signal();
#ifdef COMMAND_PROCESSOR
    int count_ = DrvAPI::atomic_add(&count(), 1);
#else
    int count_ = atomic_fetch_add_i32(&count(), 1);
#endif
    if (count_ == threads()-1) {
        count() = 0;
        f();
        signal() = !signal_;
    } else {
        static constexpr int backoff_limit = 1000;
        int backoff_counter = 8;
        while (signal() == signal_) {
            wait(backoff_counter);
            backoff_counter = std::min(backoff_counter*2, backoff_limit);
        }
    }
  }

REF_CLASS_END(barrier_data)

using barrier_ref = barrier_data_ref;
/**
 * @brief The frontier_data struct
 * This struct is used to store frontier data.
 */
struct frontier_data {
    int64_t          size;
    vertex_pointer_t vertices;
    bool             is_dense;
};

REF_CLASS_BEGIN(frontier_data)
REF_CLASS_DATA_MEMBER(frontier_data, size)
REF_CLASS_DATA_MEMBER(frontier_data, vertices)
REF_CLASS_DATA_MEMBER(frontier_data, is_dense)
vertex_ref_t vertices(vertex_t i) {
    vertex_pointer_t v = vertices();
    return v[i];
}

frontier_data_ref to_sparse(frontier_data_ref & tmp_frontier, barrier_ref barrier, vertex_t V) {
    if (is_dense()) {
        barrier.sync([=](){
            tmp_frontier.size() = 0;
            tmp_frontier.is_dense() = false;
        });
        // insert sparse data into output frontier
        for (vertex_t v = my_thread(); v < V; v += threads()) {
            if (vertices(v) == 1) {
                //pr_dbg("vertices(%d) = %d\n", v, vertices(v));
#ifdef COMMAND_PROCESSOR
                int64_t i = DrvAPI::atomic_add(&tmp_frontier.size(), 1);
#else
                int64_t i = atomic_fetch_add_i64(&tmp_frontier.size(), 1);
#endif
                tmp_frontier.vertices(i) = v;
            }
        }
        barrier.sync();
        frontier_data_ref tmp = tmp_frontier;
        tmp_frontier = *this;
        return tmp;
    }
    return *this;
}

frontier_data_ref to_dense(frontier_data_ref &tmp_frontier, barrier_ref barrier, vertex_t V) {
    if (!is_dense()) {
        // zero output frontier if needed
        if (tmp_frontier.size() != 0) {
            for (int v = my_thread(); v < V; v += threads()) {
                tmp_frontier.vertices(v) = 0;
            }
        }
        barrier.sync([=](){
            tmp_frontier.size() = size();
            tmp_frontier.is_dense() = true;
        });
        // insert sparse data into output frontier
        for (int v_i = my_thread(); v_i < size(); v_i += threads()) {
            int v = vertices(v_i);
            tmp_frontier.vertices(v) = 1;
        }
        barrier.sync();
        frontier_data_ref tmp = tmp_frontier;
        tmp_frontier = *this;
        return tmp;
    }
    return *this;
}

void clear(barrier_ref barrier, vertex_t V) {
    barrier.sync();
    for (vertex_t v = my_thread(); v < V; v += threads()) {
        vertices(v) = 0;
    }
    barrier.sync([=](){
        size() = 0;
        is_dense() = true;
    });
}

REF_CLASS_END(frontier_data)

using frontier_ref = frontier_data_ref;

#ifndef COMMAND_PROCESSOR
/**
 * swap to frontier references locally
 */
static void swap(frontier_ref &a, frontier_ref &b)
{
    frontier_data_ref tmp = a;
    a = b;
    b = tmp;
}
#endif
