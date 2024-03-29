// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include "DrvSimpleMemory.hpp"
#include "DrvCore.hpp"
using namespace SST;
using namespace Drv;

/**
 * @brief Construct a new DrvSimpleMemory object
 */
DrvSimpleMemory::DrvSimpleMemory(SST::ComponentId_t id, SST::Params& params, DrvCore *core)
    : DrvMemory(id, params, core) {
    int size = params.find<int>("size", 1024);
    if (size <= 0) {
        output_.fatal(CALL_INFO, -1, "Memory size must be positive\n");
    }
    data_.resize(size);
    output_.verbose(CALL_INFO, 1, DrvMemory::VERBOSE_INIT, "constructor done\n");
}


/**
 * @brief Send a read request
 */
void
DrvSimpleMemory::sendReadRequest(DrvCore *core, DrvThread *thread, const std::shared_ptr<DrvAPI::DrvAPIMemRead> &read_req) {
    output_.verbose(CALL_INFO, 1, DrvMemory::VERBOSE_REQ, "sending read request\n");
    read_req->setResult(&data_[read_req->getAddress()]);
    read_req->complete();
}

/**
 * @brief Send a write request
 */
void
DrvSimpleMemory::sendWriteRequest(DrvCore *core, DrvThread *thread, const std::shared_ptr<DrvAPI::DrvAPIMemWrite> &write_req) {
    output_.verbose(CALL_INFO, 1, DrvMemory::VERBOSE_REQ, "sending write request\n");
    write_req->getPayload(&data_[write_req->getAddress()]);
    write_req->complete();
}

/**
 * @brief Send an atomic request
 */
void
DrvSimpleMemory::sendAtomicRequest(DrvCore *core
                                   ,DrvThread *thread
                                   ,const std::shared_ptr<DrvAPI::DrvAPIMemAtomic> &atomic_req) {
    output_.verbose(CALL_INFO, 1, DrvMemory::VERBOSE_REQ, "sending atomic request\n");
    atomic_req->setResult(&data_[atomic_req->getAddress()]);
    atomic_req->modify();
    atomic_req->getPayload(&data_[atomic_req->getAddress()]);
    atomic_req->complete();
}

/**
 * @brief Send a memory request
 */
void
DrvSimpleMemory::sendRequest (DrvCore *core, DrvThread *thread, const std::shared_ptr<DrvAPI::DrvAPIMem> & thread_mem_req) {
    auto read_req = std::dynamic_pointer_cast<DrvAPI::DrvAPIMemRead>(thread_mem_req);
    if (read_req) {
        return sendReadRequest(core, thread, read_req);
    }

    auto write_req = std::dynamic_pointer_cast<DrvAPI::DrvAPIMemWrite>(thread_mem_req);
    if (write_req) {
        return sendWriteRequest(core, thread, write_req);
    }

    auto atomic_req = std::dynamic_pointer_cast<DrvAPI::DrvAPIMemAtomic>(thread_mem_req);
    if (atomic_req) {
        return sendAtomicRequest(core, thread, atomic_req);
    }
    core_->assertCoreOn();
    return;
}
