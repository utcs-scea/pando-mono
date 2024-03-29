// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#pragma once
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <string>
#include "DrvMemory.hpp"
#include "DrvEvent.hpp"
#include "DrvAPIThreadState.hpp"

namespace SST {
namespace Drv {

/**
 * @brief The memory class using a self-link to model latency
 *
 */
class DrvSelfLinkMemory : public DrvMemory {
public:

    // register this subcomponent into the element library
    SST_ELI_REGISTER_SUBCOMPONENT(
        SST::Drv::DrvSelfLinkMemory,
        "Drv",
        "DrvSelfLinkMemory",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Memory with fixed latency",
        SST::Drv::DrvMemory
    )

    // document the parameters that this component accepts
    SST_ELI_DOCUMENT_PARAMS(
        {"size", "Size of the memory", "0"},
    )
    // register ports
    SST_ELI_DOCUMENT_PORTS(
        {"port", "Self link to memory", {"Drv.DrvSelfLinkMemory.Event"}},
    )
    /**
     * @brief Construct a new DrvSelfLinkMemory object
     */
    DrvSelfLinkMemory(SST::ComponentId_t id, SST::Params& params, DrvCore *core);

    /**
     * @brief Destroy the DrvSelfLinkMemory object
     */
    virtual ~DrvSelfLinkMemory() {}

    /**
     * @brief Send a memory request
     */
    void sendRequest(DrvCore *core
                     ,DrvThread *thread
                     ,const std::shared_ptr<DrvAPI::DrvAPIMem> &mem_req);

    /**
     * Event class for self-link
     */
    class Event : public SST::Event {
    public:
        /**
         * @brief Construct a new DrvEvent object
         */
        Event() {}

        /**
         * @brief Destroy the DrvEvent object
         */
        virtual ~Event() {}

        std::shared_ptr<DrvAPI::DrvAPIMem> req_; //!< The memory request

        ImplementSerializable(SST::Drv::DrvSelfLinkMemory::Event);
    };

private:
    /**
     * handleEvent is called when a message is received
     */
    void handleEvent(SST::Event *ev);

    SST::Link  *link_; //!< A link
    DrvCore    *core_; //!< The core
    std::vector<uint8_t> data_; //!< The data store
};

}
}
