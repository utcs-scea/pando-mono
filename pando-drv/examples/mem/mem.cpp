// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <DrvAPI.hpp>
#include <inttypes.h>
#include <stdio.h>
using namespace DrvAPI;

DrvAPIGlobalL2SP<uint64_t> g_ul2sp0;
DrvAPIGlobalL2SP<uint64_t> g_ul2sp1;
DrvAPIGlobalL2SP<int64_t> g_sl2sp;

int MemMain(int argc, char* argv[]) {
  printf("Hello from %s\n", __PRETTY_FUNCTION__);

  DrvAPIAddress addr = &g_ul2sp0;

  uint64_t writeval = 0xdeadbeefcafebabe;
  printf("writing %lx\n", writeval);
  DrvAPI::write<uint64_t>(addr, writeval);
  uint64_t readback = DrvAPI::read<uint64_t>(addr);
  printf("wrote %lx, read back %lx\n", writeval, readback);

  DrvAPIAddress addr1 = &g_ul2sp1;
  writeval = 0xa5a5a5a5a5a5a5a5;
  printf("swapping %lx into memory\n", writeval);
  uint64_t swapback = DrvAPI::atomic_swap<uint64_t>(addr1, writeval);
  printf("swapped %lx, read back %lx\n", writeval, swapback);

  writeval = ~writeval;
  printf("swapping %lx into memory\n", writeval);
  swapback = DrvAPI::atomic_swap<uint64_t>(addr1, writeval);
  printf("swapped %lx, read back %lx\n", writeval, swapback);

  DrvAPIAddress addr2 = &g_sl2sp;
  writeval = 2;
  printf("writing %" PRId64 " to memory\n", writeval);
  DrvAPI::write<uint64_t>(addr2, writeval);
  printf("adding %d to memory\n", -1);
  int64_t addback = DrvAPI::atomic_add<int64_t>(addr2, -1);
  printf("added %d, read back %" PRId64 "\n", -1, addback);
  printf("adding %d to memory\n", -1);
  addback = DrvAPI::atomic_add<int64_t>(addr2, -1);
  printf("added %d, read back %" PRId64 "\n", -1, addback);

  printf("done!\n");
  return 0;
}

declare_drv_api_main(MemMain);
