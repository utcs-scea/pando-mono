// Copyright 2018-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2018-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _STS_H
#define _STS_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <vector>
#include <queue>

#include <sst/core/interfaces/stdMem.h>

#include "neuron.h"


namespace SST {
namespace GNAComponent {

//foward decl
class GNA;
class Request;

//  A Spike Transfer Structure engine - transforms a given spike by
//  performing a look up and delivering spikes
class STS {
public:
    GNA * myGNA;
    int   stsID;
    int   numSpikes; // number of spikes yet to deliver
    std::queue<SST::Interfaces::StandardMem::Request *> incomingReqs;

    STS(GNA *parent, int n);

    bool isFree       ();
    void assign       (const Neuron * n);
    void advance      (uint);
    void returnRequest(SST::Interfaces::StandardMem::Request *req);
};

}
}

#endif // _STS_H
