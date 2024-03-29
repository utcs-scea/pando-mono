// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include "DrvSelfLinkMemory.hpp"
#include "DrvCore.hpp"
#include "DrvEvent.hpp"
#include <cstdio>

using namespace SST;
using namespace Drv;

DrvSelfLinkMemory::DrvSelfLinkMemory(SST::ComponentId_t id, SST::Params& params, DrvCore *core)
    : DrvMemory(id, params, core) {
    int size = params.find<int>("size", 0);
    if (size <= 0) {
        output_.fatal(CALL_INFO, -1, "ERROR: %s: invalid size\n", __FUNCTION__);
    }
    data_.resize(size);
    // configure link
    link_ = configureLink("port", new Event::Handler<DrvSelfLinkMemory>(this, &DrvSelfLinkMemory::handleEvent));
    output_.verbose(CALL_INFO, 1, DrvMemory::VERBOSE_INIT, "constructor done\n");
}


void DrvSelfLinkMemory::handleEvent(SST::Event *ev) {
  core_->output()->verbose(CALL_INFO, 2, DrvMemory::VERBOSE_REQ, "Received event\n");
  auto mem_evt = dynamic_cast<DrvSelfLinkMemory::Event*>(ev);
  if (!mem_evt) {
    core_->output()->fatal(CALL_INFO, -1, "ERROR: %s: invalid event type\n", __FUNCTION__);
    return;
  }

  std::shared_ptr<DrvAPI::DrvAPIMem> mem_req = mem_evt->req_;

  auto read = std::dynamic_pointer_cast<DrvAPI::DrvAPIMemRead>(mem_req);
  if (read) {
    read->setResult(&data_[read->getAddress()]);
    read->complete();
    return;
  }

  auto write = std::dynamic_pointer_cast<DrvAPI::DrvAPIMemWrite>(mem_req);
  if (write) {
    write->getPayload(&data_[write->getAddress()]);
    write->complete();
    return;
  }

  auto atomic = std::dynamic_pointer_cast<DrvAPI::DrvAPIMemAtomic>(mem_req);
  if (atomic) {
    atomic->setResult(&data_[atomic->getAddress()]);
    atomic->modify();
    atomic->getPayload(&data_[atomic->getAddress()]);
    atomic->complete();
    return;
  }
  core_->assertCoreOn();
  delete ev;
}


void DrvSelfLinkMemory::sendRequest(DrvCore *core
                                    ,DrvThread *thread
                                    ,const std::shared_ptr<DrvAPI::DrvAPIMem> &mem_req) {
  core->output()->verbose(CALL_INFO, 2, DrvMemory::VERBOSE_REQ, "Sending request\n");
  auto ev = new DrvSelfLinkMemory::Event();
  ev->req_ = mem_req;
  link_->send(0, ev);
}
