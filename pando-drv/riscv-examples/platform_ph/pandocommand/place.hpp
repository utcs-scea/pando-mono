// SPDX-License-Identifier: MIT
// Copyright (c) 2024 University of Washington
#include <cstdint>
namespace pandocommand {

struct Place {
    int64_t pxn;
    int64_t pod;
    int64_t core_y;
    int64_t core_x;
    Place(): pxn(0), pod(0), core_y(0), core_x(0) {}
    Place(int64_t pxn, int64_t pod, int64_t core_y, int64_t core_x):
        pxn(pxn), pod(pod), core_y(core_y), core_x(core_x) {}
};

} // namespace pandocommand
