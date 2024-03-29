// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#ifndef DRV_API_SYSTEM_HPP
#define DRV_API_SYSTEM_HPP
#include "DrvAPIAddress.hpp"
#include <stdexcept>
namespace DrvAPI
{
/**
 * @brief System-level API
 *
 * This class provides system services
 * It is meant to be implemented by a simulator
 *
 * Note that the other way is request services from the system
 * is by yielding with a state.
 *
 * This can avoid the cost of a context switch and avoid simulation time passing.
 * These functions are also safe to call regardless of whether caller is executing
 * from a thread context or simulator.
 */
class DrvAPISystem
{
public:
    DrvAPISystem() = default;
    DrvAPISystem(const DrvAPISystem &) = delete;
    DrvAPISystem &operator=(const DrvAPISystem &) = delete;
    DrvAPISystem(DrvAPISystem &&) = default;
    DrvAPISystem &operator=(DrvAPISystem &&) = default;
    virtual ~DrvAPISystem() = default;
    /**
     * @brief Convert a DrvAPIAddress to a native pointer
     *
     * WARNING:
     * This function will not work in multi-rank simulations.
     * This function may not work depending on the memory model used.
     * This function may not work depending on the memory controller used.
     *
     * Avoid using this function if possible. But if you need to use it, it's here.
     * But use it at your own risk, and don't expect it to work for all memory models
     * and simulation configurations.
     *
     * @param address the simulator address
     * @param native the native pointer returned from translation
     * @param the number of valid bytes starting at the native pointer
     *
     * It is optional to support this function.
     */
    virtual void
    addressToNative(DrvAPIAddress address, void **native, std::size_t *size) {
        throw std::runtime_error("adddressToNative() not implemented");
    }
    /**
     * @brief get cycle count of simulation
     */
    virtual uint64_t getCycleCount() {
        throw std::runtime_error("getCycleCount() not implemented");
    }
    /**
     * @brief get clock in hz
     */
    virtual uint64_t getClockHz() {
        throw std::runtime_error("getClockHz() not implemented");
    }
    /**
     * @brief get the simulation time in seconds
     */
    virtual double getSeconds() {
        throw std::runtime_error("getSimulationTime() not implemented");
    }
    /**
     * @brief output simulation statistics
     */
    virtual void outputStatistics(const std::string &tagname) {
        throw std::runtime_error("outputStatistics() not implemented");
    }
};
}
#endif // DRV_API_SYSTEM_HPP
