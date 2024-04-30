// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef DRV_API_MEMORY_H
#define DRV_API_MEMORY_H
#include <DrvAPIAddress.hpp>
#include <DrvAPIThread.hpp>

namespace DrvAPI
{

/**
 * @brief read from a memory address
 *
 */
template <typename T>
T read(DrvAPIAddress address, stage_t stage = STAGE_OTHER, int phase = 0)
{
    T result{};
    DrvAPIThread::current()->setState(std::make_shared<DrvAPIMemReadConcrete<T>>(address));
    DrvAPIThread::current()->setStage(stage);
    DrvAPIThread::current()->setPhase(phase);
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
void write(DrvAPIAddress address, T value, stage_t stage = STAGE_OTHER, int phase = 0)
{
    DrvAPIThread::current()->setState(std::make_shared<DrvAPIMemWriteConcrete<T>>(address, value));
    DrvAPIThread::current()->setStage(stage);
    DrvAPIThread::current()->setPhase(phase);
    DrvAPIThread::current()->yield();
}

/**
 * @brief atomic swap to a memory address
 *
 */
template <typename T>
T atomic_swap(DrvAPIAddress address, T value, stage_t stage = STAGE_OTHER, int phase = 0)
{
    T result = 0;
    DrvAPIThread::current()->setState(std::make_shared<DrvAPIMemAtomicConcrete<T, DrvAPIMemAtomicSWAP>>(address, value));
    DrvAPIThread::current()->setStage(stage);
    DrvAPIThread::current()->setPhase(phase);
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
T atomic_add(DrvAPIAddress address, T value, stage_t stage = STAGE_OTHER, int phase = 0)
{
    T result = 0;
    DrvAPIThread::current()->setState(std::make_shared<DrvAPIMemAtomicConcrete<T, DrvAPIMemAtomicADD>>(address, value));
    DrvAPIThread::current()->setStage(stage);
    DrvAPIThread::current()->setPhase(phase);
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
T atomic_or(DrvAPIAddress address, T value, stage_t stage = STAGE_OTHER, int phase = 0)
{
    T result = 0;
    DrvAPIThread::current()->setState(std::make_shared<DrvAPIMemAtomicConcrete<T, DrvAPIMemAtomicOR>>(address, value));
    DrvAPIThread::current()->setStage(stage);
    DrvAPIThread::current()->setPhase(phase);
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
T atomic_cas(DrvAPIAddress address, T compare, T value, stage_t stage = STAGE_OTHER, int phase = 0)
{
    T result = 0;
    DrvAPIThread::current()->setState(std::make_shared<DrvAPIMemAtomicConcreteExt<T, DrvAPIMemAtomicCAS>>(address, value, compare));
    DrvAPIThread::current()->setStage(stage);
    DrvAPIThread::current()->setPhase(phase);
    DrvAPIThread::current()->yield();
    auto atomic_req = std::dynamic_pointer_cast<DrvAPIMemAtomic>(DrvAPIThread::current()->getState());
    if (atomic_req) {
        atomic_req->getResult(&result);
    }
    return result;
}

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
