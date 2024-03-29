// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include "DrvNativeSimulationTranslator.hpp"
#include "rv64simtypes/stat.h"
#include "rv64simtypes/fcntl.h"

using namespace SST;
using namespace Drv;

std::vector<unsigned char>
DrvNativeSimulationTranslator::nativeToSimulator_stat(const struct stat *i) {
    std::vector<unsigned char> ret(sizeof(rv64sim_stat_t));
    rv64sim_stat_t *o = reinterpret_cast<rv64sim_stat_t *>(&ret[0]);
    o->st_dev = i->st_dev;
    o->st_ino = i->st_ino;
    o->st_mode = i->st_mode;
    o->st_nlink = i->st_nlink;
    o->st_uid = i->st_uid;
    o->st_gid = i->st_gid;
    o->st_rdev = i->st_rdev;
    o->st_size = i->st_size;
    o->st_atim = {};
    o->st_mtim = {};
    o->st_ctim = {};
    o->st_blksize = i->st_blksize;
    o->st_blocks = i->st_blocks;
    return ret;
}


int DrvNativeSimulationTranslator::simulatorToNative_openflags(int32_t sim_openflags) {
    int ret = 0;
    if (sim_openflags & RV64SIM_O_RDONLY) {
        ret |= O_RDONLY;
    }
    if (sim_openflags & RV64SIM_O_WRONLY) {
        ret |= O_WRONLY;
    }
    if (sim_openflags & RV64SIM_O_RDWR) {
        ret |= O_RDWR;
    }
    if (sim_openflags & RV64SIM_O_CREAT) {
        ret |= O_CREAT;
    }
    if (sim_openflags & RV64SIM_O_TRUNC) {
        ret |= O_TRUNC;
    }
    return ret;
}
