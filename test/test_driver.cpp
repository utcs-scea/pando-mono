// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <pando-lib-galois/containers/host_local_storage.hpp>
#include <pando-lib-galois/containers/pod_local_storage.hpp>
#include <pando-rt/pando-rt.hpp>

#ifdef ENABLE_GCOV
extern const struct gcov_info* const __gcov_info_start[];
extern const struct gcov_info* const __gcov_info_end[];

extern "C" void __gcov_init(const struct gcov_info*);
extern "C" void __gcov_exit(void);
#endif

int pandoMain(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

#ifdef ENABLE_GCOV
  const struct gcov_info* const* info = __gcov_info_start;
  const struct gcov_info* const* end = __gcov_info_end;
  while (info != end) {
    __gcov_init(*info);
    ++info;
  }
#endif

  int result = 0;
  if (pando::getCurrentPlace().node.id == 0) {
    galois::HostLocalStorageHeap::HeapInit();
    galois::PodLocalStorageHeap::HeapInit();
    result = RUN_ALL_TESTS();
  }
  pando::waitAll();

#ifdef ENABLE_GCOV
  __gcov_exit();
#endif

  return result;
}
