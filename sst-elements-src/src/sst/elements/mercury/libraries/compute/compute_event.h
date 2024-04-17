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

#ifndef SSTMAC_HARDWARE_PROCESSOR_EVENTDATA_H_INCLUDED
#define SSTMAC_HARDWARE_PROCESSOR_EVENTDATA_H_INCLUDED


#include <sstmac/common/timestamp.h>
#include <sstmac/common/sst_event.h>
#include <sstmac/hardware/common/flow.h>
#include <sstmac/hardware/memory/memory_id.h>
#include <type_traits>
#include <sprockit/debug.h>
#include <sprockit/typedefs.h>
#include <sprockit/thread_safe_new.h>
#include <stdint.h>

DeclareDebugSlot(compute_intensity);

namespace sstmac {
namespace sw {

/**
 * Input for processor models that use
 * performance counter data. Is basically just a map
 * that maps std::string keys to integer values.
 * Keys are defined in the libraries that use them.
 */
class ComputeEvent :
 public Event
{
 public:
  virtual bool isTimedCompute() const = 0;

  void setCore(int core){
    core_ = core;
  }

  std::string toString() const {
    return "compute event";
  }

  int core() const {
    return core_;
  }

  hw::MemoryAccessId accessId() const {
    return unique_id_;
  }

  void setAccessId(hw::MemoryAccessId id) {
    unique_id_ = id;
  }

  uint64_t uniqueId() const {
    return uint64_t(unique_id_);
  }

 private:
  int core_;

  hw::MemoryAccessId unique_id_;

};

template <class T>
class ComputeEvent_impl :
 public ComputeEvent,
 public sprockit::thread_safe_new<ComputeEvent_impl<T>>
{
  NotSerializable(ComputeEvent_impl)

 public:
  bool isTimedCompute() const override {
    return std::is_same<T,TimeDelta>::value;
  }

  T& data() {
    return t_;
  }


 private:
  T t_;

};

struct basic_instructions_st
{
  uint64_t mem_random = 0ULL;
  uint64_t mem_sequential = 0ULL;
  uint64_t flops = 0ULL;
  uint64_t intops = 0ULL;
  int nthread = 1;
};

typedef ComputeEvent_impl<TimeDelta> TimedComputeEvent;
typedef ComputeEvent_impl<basic_instructions_st> BasicComputeEvent;

}
}  // end of namespace sstmac

#endif
