// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include <string>
#include <sstream>
#include <iomanip>
#include <inttypes.h>
#include "DrvAPIAddressMap.hpp"
namespace DrvAPI {

/**
 * @brief Translate a virtual address to a physical address
 */
DrvAPIPAddress DrvAPIVAddress::to_physical(uint32_t this_pxn
                                           ,uint32_t this_pod
                                           ,uint32_t this_core_y
                                           ,uint32_t this_core_x) {
    DrvAPIPAddress ret;
    if (is_ctrl_register()) {
        ret.type() = DrvAPIPAddress::TYPE_CTRL;
        ret.pxn() = pxn();
        ret.pod() = pod();
        ret.core_y() = core_y();
        ret.core_x() = core_x();
    } else if (not_scratchpad()) {
        ret.type() = DrvAPIPAddress::TYPE_DRAM;
        ret.pxn() = pxn();
        ret.dram_offset() = dram_offset();
    } else if (l2_not_l1()) {
        ret.type() = DrvAPIPAddress::TYPE_L2SP;
        ret.pxn() = global() ? pxn() : this_pxn;
        ret.pod() = global() ? pod() : this_pod;
        ret.l2_offset() = l2_offset();
    } else {
        ret.type() = DrvAPIPAddress::TYPE_L1SP;
        ret.pxn() = global() ? pxn() : this_pxn;
        ret.pod() = global() ? pod() : this_pod;
        ret.core_y() = global() ? core_y() : this_core_y;
        ret.core_x() = global() ? core_x() : this_core_x;
        ret.l1_offset() = l1_offset();
    }
    return ret;
}

DrvAPIPAddress DrvAPIVAddress::to_physical(DrvAPIAddress addr
                                           ,uint32_t this_pxn
                                           ,uint32_t this_pod
                                           ,uint32_t this_core_y
                                           ,uint32_t this_core_x) {
    return DrvAPIVAddress{addr}.to_physical(this_pxn, this_pod, this_core_y, this_core_x);
}


std::string DrvAPIVAddress::to_string() {
    std::stringstream type;
    std::stringstream locale;
    std::stringstream offset;
    offset << "0x" << std::setfill('0') << std::setw(11) << std::hex;
    if (is_ctrl_register()) {
        type << "CTRL";
        locale << "PXN=" << pxn() << " POD=" << pod();
        locale << " CORE_Y=" << core_y() << " CORE_X=" << core_x();
        offset << ctrl_offset();
    } else if (not_scratchpad()) {
        type << "DRAM";
        locale << "PXN=" << pxn();
        offset << dram_offset();
    } else if (l2_not_l1()) {
        type << "L2SP";
        if (global()) {
            locale << "PXN=" << pxn() << " POD=" << pod();
        } else {
            locale << "LOCAL";
        }
        offset << l2_offset();
    } else {
        type << "L1SP";
        if (global()) {
            locale << "PXN=" << pxn() << " POD=" << pod() << " CORE_Y=" << core_y() << " CORE_X=" << core_x();
        } else {
            locale << "LOCAL";
        }
        offset << l1_offset();
    }
    return "VADDR{" + type.str() + " " + locale.str() + " " + offset.str() + "}";
}


std::string DrvAPIPAddress::to_string() {
    std::stringstream types;
    std::stringstream locale;
    std::stringstream offset;
    offset << "0x" << std::setfill('0') << std::setw(11) << std::hex;
    if (type() == TYPE_DRAM) {
        types << "DRAM";
        locale << "PXN=" << pxn();
        offset << dram_offset();
    } else if (type() == TYPE_L2SP) {
        types << "L2SP";
        locale << "PXN=" << pxn() << " POD=" << pod();
        offset << l2_offset();
    } else if (type() == TYPE_L1SP) {
        types << "L1SP";
        locale << "PXN=" << pxn() << " POD=" << pod() << " CORE_Y=" << core_y() << " CORE_X=" << core_x();
        offset << l1_offset();
    } else if (type() == TYPE_CTRL) {
        types << "CTRL";
        locale << "PXN=" << pxn() << " POD=" << pod();
        locale << " CORE_Y=" << core_y() << " CORE_X=" << core_x();
        offset << ctrl_offset();
    }
    return "PADDR{" + types.str() + " " + locale.str() + " " + offset.str() + "}";
}

} // namespace DrvAPI
