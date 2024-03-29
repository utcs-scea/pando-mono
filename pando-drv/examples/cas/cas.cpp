// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include <DrvAPI.hpp>
#include <cstdio>
#include <inttypes.h>
#include <string>
using namespace DrvAPI;

DrvAPIGlobalL2SP<int64_t> lock;

int CASMain(int argc, char* argv[]) {
  std::string wstr = "42";
  std::string cstr = "71";
  std::string istr = "0";
  if (argc > 1) {
    wstr = argv[1];
  }
  if (argc > 2) {
    cstr = argv[2];
  }
  if (argc > 3) {
    istr = argv[3];
  }
  int64_t w = std::stoll(wstr);
  int64_t c = std::stoll(cstr);
  int64_t i = std::stoll(istr);
  printf("w = %" PRId64 " ,c = %" PRId64 ", i = %" PRId64 "\n", w, c, i);
  lock = i;
  int64_t r = DrvAPI::atomic_cas(&lock, c, w);
  printf("CAS(%" PRIx64 ", %" PRId64 ", %" PRId64 ")=%" PRId64 "\n", static_cast<uint64_t>(&lock),
         w, c, r);
  printf("LOAD(%" PRIx64 ") = %" PRId64 "\n", static_cast<uint64_t>(&lock),
         static_cast<int64_t>(lock));
  return 0;
}

declare_drv_api_main(CASMain);
