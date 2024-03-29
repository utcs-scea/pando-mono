// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#pragma once

namespace SST {
namespace Drv {

#define DEFINE_DRV_STAT(name, ...)              \
    name,

enum DrvStatId {
    #include "DrvStatsTable.hpp"
};

#undef DEFINE_DRV_STAT

}
}
