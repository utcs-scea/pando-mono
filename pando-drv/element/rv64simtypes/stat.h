// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef RV64SIMTYPES_STAT_H
#define RV64SIMTYPES_STAT_H
#include "rv64simtypes/types.h"

typedef struct rv64sim_stat {
    rv64_dev_t   st_dev;
    rv64_ino_t   st_ino;
    rv64_mode_t  st_mode;
    rv64_nlink_t st_nlink;
    rv64_uid_t   st_uid;
    rv64_gid_t   st_gid;
    rv64_dev_t   st_rdev;
    rv64_off_t   st_size;
    rv64_struct_timespec  st_atim;
    rv64_struct_timespec  st_mtim;
    rv64_struct_timespec  st_ctim;
    rv64_blksize_t st_blksize;
    rv64_blkcnt_t  st_blocks;
    uint64_t st_spare4[2];
} rv64sim_stat_t;

#endif //RV64SIMTYPES_H
