// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef DRV_API_MEMORY_H
#define DRV_API_MEMORY_H
#include <DrvAPIAddress.hpp>
#include <DrvAPIAddressToNative.hpp>
#include <DrvAPIThread.hpp>

namespace DrvAPI
{

/**
 * @brief read from a memory address
 *
 */
template <typename T>
T read(DrvAPIAddress address)
{
    T result{};
    DrvAPIThread::current()->setState(std::make_shared<DrvAPIMemReadConcrete<T>>(address));
    DrvAPIThread::current()->yield();
    auto read_req = std::dynamic_pointer_cast<DrvAPIMemRead>(DrvAPIThread::current()->getState());
    if (read_req) {
        read_req->getResult(&result);
    }
    return result;
}

/**
 * @brief write to a memory address
 *
 */
template <typename T>
void write(DrvAPIAddress address, T value)
{
    DrvAPIThread::current()->setState(std::make_shared<DrvAPIMemWriteConcrete<T>>(address, value));
    DrvAPIThread::current()->yield();
}

/**
 * @brief atomic swap to a memory address
 *
 */
template <typename T>
T atomic_swap(DrvAPIAddress address, T value)
{
    T result = 0;
    DrvAPIThread::current()->setState(std::make_shared<DrvAPIMemAtomicConcrete<T, DrvAPIMemAtomicSWAP>>(address, value));
    DrvAPIThread::current()->yield();
    auto atomic_req = std::dynamic_pointer_cast<DrvAPIMemAtomic>(DrvAPIThread::current()->getState());
    if (atomic_req) {
        atomic_req->getResult(&result);
    }
    return result;
}

/**
 * @brief atomic add to a memory address
 */
template <typename T>
T atomic_add(DrvAPIAddress address, T value)
{
    T result = 0;
    DrvAPIThread::current()->setState(std::make_shared<DrvAPIMemAtomicConcrete<T, DrvAPIMemAtomicADD>>(address, value));
    DrvAPIThread::current()->yield();
    auto atomic_req = std::dynamic_pointer_cast<DrvAPIMemAtomic>(DrvAPIThread::current()->getState());
    if (atomic_req) {
        atomic_req->getResult(&result);
    }
    return result;
}

/**
 * @brief atomic add to a memory address
 */
template <typename T>
T atomic_or(DrvAPIAddress address, T value)
{
    T result = 0;
    DrvAPIThread::current()->setState(std::make_shared<DrvAPIMemAtomicConcrete<T, DrvAPIMemAtomicOR>>(address, value));
    DrvAPIThread::current()->yield();
    auto atomic_req = std::dynamic_pointer_cast<DrvAPIMemAtomic>(DrvAPIThread::current()->getState());
    if (atomic_req) {
        atomic_req->getResult(&result);
    }
    return result;
}

/**
 * @brief atomic compare and swap to a memory address
 */
template <typename T>
T atomic_cas(DrvAPIAddress address, T compare, T value)
{
    T result = 0;
    DrvAPIThread::current()->setState(std::make_shared<DrvAPIMemAtomicConcreteExt<T, DrvAPIMemAtomicCAS>>(address, value, compare));
    DrvAPIThread::current()->yield();
    auto atomic_req = std::dynamic_pointer_cast<DrvAPIMemAtomic>(DrvAPIThread::current()->getState());
    if (atomic_req) {
        atomic_req->getResult(&result);
    }
    return result;
}

/**
 * @brief monitor a memory address until it reaches a certain value
 */
template <typename T>
void monitor_until(DrvAPIAddress address, T value)
{
    DrvAPIThread::current()->setState(std::make_shared<DrvAPIMemMonitorUntil<T>>(address, value, true));
    DrvAPIThread::current()->yield();
}

/**
 * @brief monitor a memory address until it is no longer a certain value
 */
template <typename T>
void monitor_until_not(DrvAPIAddress address, T value)
{
    DrvAPIThread::current()->setState(std::make_shared<DrvAPIMemMonitorUntil<T>>(address, value, false));
    DrvAPIThread::current()->yield();
}

/**
 * @brief set the stage
 *
 */
extern void set_stage(stage_t stage);

/**
 * @brief increment the phase
 *
 */
extern void increment_phase();

/**
 * @brief memory fence
 *
 */
inline void fence()
{
    // TODO: change when we add non-blocking memory ops
    return;
}

} // namespace DrvAPI

#endif
