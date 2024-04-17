// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst/core/params.h>
#include <components/operating_system.h>
//#include <libraries/compute/lib_compute_memmove.h>
#include <operating_system/process/thread.h>
#include <operating_system/process/app.h>
#include <operating_system/libraries/api.h>
#include <hardware/common/flow.h>
//#include <sstmac/common/sstmac_env.h>
#include <common/thread_lock.h>
//#include <sprockit/keyword_registration.h>

namespace SST {
namespace Hg {

extern template class  HgBase<SST::Component>;
extern template class  HgBase<SST::SubComponent>;
extern template SST::TimeConverter* HgBase<SST::SubComponent>::time_converter_;

static thread_lock the_api_lock;

void
apiLock() {
  the_api_lock.lock();
}

void
apiUnlock() {
  the_api_lock.unlock();
}

API::~API()
{
}

SST::Hg::SoftwareId
API::sid() const {
  return parent_->sid();
}

NodeId
API::addr() const {
  return parent_->os()->addr();
}

Thread*
API::activeThread()
{
  return parent_->os()->activeThread();
}

void
API::startAPICall()
{
//  if (host_timer_){
//    host_timer_->start();
//  }
  activeThread()->startAPICall();
}
void
API::endAPICall()
{
//  if (host_timer_) {
//    double time = host_timer_->stamp();
//    parent_->compute(TimeDelta(time));
//  }
  activeThread()->endAPICall();
}

Timestamp
API::now() const 
{
  return parent_->os()->now();
}

void
API::schedule(Timestamp t, ExecutionEvent* ev)
{
  parent_->os()->sendExecutionEvent(t, ev);
}

void
API::scheduleDelay(TimeDelta t, ExecutionEvent* ev)
{
  parent_->os()->sendDelayedExecutionEvent(t, ev);
}

API::API(SST::Params & /*params*/, App *parent, SST::Component*  /*comp*/) :
  parent_(parent)
{ }

} // end namespace Hg
} // end namespace SST
