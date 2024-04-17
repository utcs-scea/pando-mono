// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
// Description of modifications made by AMD:
// 1) Added userID member variable to ArielReadEvent class and parameter to constructor.

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


#ifndef _H_SST_ARIEL_READ_EVENT
#define _H_SST_ARIEL_READ_EVENT

#include "arielevent.h"

using namespace SST;

namespace SST {
namespace ArielComponent {

class ArielReadEvent : public ArielEvent {

    public:
        ArielReadEvent(uint64_t rAddr, uint32_t length, uint64_t userID) :
                readAddress(rAddr), readLength(length), userID(userID) {
        }

        ~ArielReadEvent() {
        }

        ArielEventType getEventType() const {
                return READ_ADDRESS;
        }

        uint64_t getAddress() const {
                return readAddress;
        }

        uint32_t getLength() const {
                return readLength;
        }

        int getuserID() const {
                return userID;
        }

    private:
        const uint64_t readAddress;
        const uint32_t readLength;
        const uint64_t userID;

};

}
}

#endif
