// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include "DrvAPINativeToAddress.hpp"
#include "DrvAPIThread.hpp"

namespace DrvAPI {

void DrvAPINativeToAddress(void *native, DrvAPIAddress *address, std::size_t *size) {
    DrvAPIThread::current()->nativeToAddress(native, address, size);
}

}
