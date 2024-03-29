// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#pragma once
#include <DrvMemory.hpp>
#include <string>
#include <vector>

namespace SST {
namespace Drv {

/**
 * @brief A simple memory class
 *
 * This memory class is has a constant latency and built-in data store.
 */
class DrvSimpleMemory : public DrvMemory {
public:
    // register this subcomponent into the element library
    SST_ELI_REGISTER_SUBCOMPONENT(
        SST::Drv::DrvSimpleMemory,
        "Drv",
        "DrvSimpleMemory",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Simple memory with no latency",
        SST::Drv::DrvMemory
    )
    // document the parameters that this component accepts
    SST_ELI_DOCUMENT_PARAMS(
        {"size", "The size of the memory", "1024"},
    )

    /**
     * @brief Construct a new DrvSimpleMemory object
     */
    DrvSimpleMemory(SST::ComponentId_t id, SST::Params& params, DrvCore *core);

    /**
     * @brief Destroy the DrvSimpleMemory object
     */
    virtual ~DrvSimpleMemory() {}

    /**
     * @brief Send a memory request
     */
    virtual void sendRequest(DrvCore *core, DrvThread *thread, const std::shared_ptr<DrvAPI::DrvAPIMem> & thread_mem_req) override;

private:
    /**
     * @brief Send a read request
     */
    void sendWriteRequest(DrvCore *core
                          ,DrvThread *thread
                          ,const std::shared_ptr<DrvAPI::DrvAPIMemWrite> &write_req);

    /**
     * @brief Send a write request
     */
    void sendReadRequest(DrvCore *core
                         ,DrvThread *thread
                         ,const std::shared_ptr<DrvAPI::DrvAPIMemRead> &read_req);

    /**
     * @brief Send an atomic request
     */
    void sendAtomicRequest(DrvCore *core
                           ,DrvThread *thread
                           ,const std::shared_ptr<DrvAPI::DrvAPIMemAtomic> &atomic_req);

    // members
    std::vector<uint8_t> data_; //!< The data store
};

}
}
