// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.

#include <DrvAPI.hpp>
#include <cstdint>
#include <inttypes.h>
#include <string>

using namespace DrvAPI;

int GupsMain(int argc, char* argv[]) {
  int pxns = numPXNs();
  DrvAPIAddress TABLE[pxns];
  for (int pxn = 0; pxn < pxns; pxn++) {
    auto base = DrvAPIVAddress::MainMemBase(pxn);
    TABLE[pxn] = base.encode();
  }

  std::string tbl_size_str = "1048576";
  std::string thread_n_updates_str = "1024";
  if (argc > 1) {
    tbl_size_str = argv[1];
  }
  if (argc > 2) {
    thread_n_updates_str = argv[2];
  }
  int64_t tbl_size = std::stoll(tbl_size_str);
  int64_t thread_n_updates = std::stoll(thread_n_updates_str);

  if (DrvAPIThread::current()->coreId() == 0 && DrvAPIThread::current()->threadId() == 0 &&
      myPodId() == 0) {
    printf("Core %4d: Thread %4d: pod %4d: pxn %4d, tbl_size = %" PRId64
           ", thread_n_updates = %" PRId64 "\n",
           DrvAPIThread::current()->coreId(), DrvAPIThread::current()->threadId(), myPodId(),
           myPXNId(), tbl_size, thread_n_updates);
    for (int pxn = 0; pxn < pxns; pxn++) {
      printf("TABLE[%4d]=%" PRIx64 " ", pxn, TABLE[pxn]);
    }
    printf("\n");
  }

  for (int64_t u = 0; u < thread_n_updates; u++) {
    int64_t i = rand() % tbl_size;
    int pxn = rand() % pxns;
    int64_t addr = TABLE[pxn] + i * sizeof(int64_t);
    auto val = read<int64_t>(addr);
    write(addr, val ^ addr);
  }

  return 0;
}

declare_drv_api_main(GupsMain);
