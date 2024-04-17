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

#ifndef _H_VANADIS_OS_SYSCALL_FUTEX
#define _H_VANADIS_OS_SYSCALL_FUTEX

#include "os/syscall/syscall.h"
#include "os/callev/voscallfutex.h"

namespace SST {
namespace Vanadis {

class VanadisFutexSyscall : public VanadisSyscall {
public:
    VanadisFutexSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallFutexEvent* event );
    ~VanadisFutexSyscall() {}

 private:  

    enum State { ReadAddr, ReadArgs } m_state;
    void memReqIsDone(bool);
    void wakeup();
    void finish( uint32_t val2, uint64_t addr2 );

    uint32_t m_val;
    bool m_waitStoreConditional;
    int m_op;
    std::vector<uint8_t> m_buffer;
    int m_numWokeup;
};

} // namespace Vanadis
} // namespace SST

#endif
