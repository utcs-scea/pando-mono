// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <utils.hpp>

#define SIZE ((uint64_t)1000)

// T = remote ref, F = executeOn
bool getAllowRemoteAccess(int argc, char** argv) {
  // Other libraries may have called getopt before, so we reset optind for correctness
  optind = 0;

  int32_t flag = 0;
  bool allowRemoteAccess = false;
  while ((flag = getopt(argc, argv, ":y")) != -1) {
    switch (flag) {
      case 'y':
        allowRemoteAccess = true;
        break;
      case 'h':
        std::cerr << "Use -y to allow remote accesses (move-data-to-compute). Default: "
                     "move-compute-to-data\n";
        std::exit(0);
      case '?':
        if (isprint(optopt))
          fprintf(stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
        std::cerr << "Use -y to allow remote accesses (move-data-to-compute). Default: "
                     "move-compute-to-data\n";
        std::exit(1);
      default:
        std::cerr << "Use -y to allow remote accesses (move-data-to-compute). Default: "
                     "move-compute-to-data\n";
        std::exit(1);
    }
  }

  return allowRemoteAccess;
}

void HBMainRemoteAccess(pando::Notification::HandleType hb_done, bool allowRemoteAccess) {
  pando::Place remote_place = pando::Place{pando::NodeIndex{1}, pando::anyPod, pando::anyCore};
  // Put data on HOST 1
  galois::Array<uint64_t> nums;
  PANDO_CHECK(nums.initialize(SIZE, remote_place, pando::MemoryType::Main));
  auto fn_put_data = +[](pando::Notification::HandleType hb_done, galois::Array<uint64_t> nums) {
    uint64_t i = 0;
    for (auto it = nums.begin(); it != nums.end(); it++)
      *it = i++;
    hb_done.notify();
  };
  pando::Notification notif_initData;
  PANDO_CHECK(notif_initData.init());
  PANDO_CHECK(pando::executeOn(remote_place, fn_put_data, notif_initData.getHandle(), nums));
  notif_initData.wait();

  // Sum data on HOST 1
  pando::GlobalPtr<uint64_t> sum_ptr = static_cast<pando::GlobalPtr<uint64_t>>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(uint64_t)));
  *sum_ptr = 0;
  if (allowRemoteAccess) {
    uint64_t sum = 0;
    for (auto it = nums.begin(); it != nums.end(); it++)
      sum += *it;
    *sum_ptr = sum;
  } else {
    pando::Notification notif_sumData;
    auto fn_sum_data = +[](pando::Notification::HandleType hb_done, galois::Array<uint64_t> nums,
                           pando::GlobalPtr<uint64_t> sum_ptr) {
      uint64_t sum = 0;
      for (auto it = nums.begin(); it != nums.end(); it++)
        sum += *it;
      *sum_ptr = sum;
      hb_done.notify();
    };
    PANDO_CHECK(notif_sumData.init());
    PANDO_CHECK(
        pando::executeOn(remote_place, fn_sum_data, notif_sumData.getHandle(), nums, sum_ptr));
    notif_sumData.wait();
  }

  std::cout << "SUM: " << *sum_ptr << "\n";

  // De-init data on HOST 1
  auto fn_dealloc_data =
      +[](pando::Notification::HandleType hb_done, galois::Array<uint64_t> nums) {
        nums.deinitialize();
        hb_done.notify();
      };
  pando::Notification notif_deleteData;
  PANDO_CHECK(notif_deleteData.init());
  PANDO_CHECK(pando::executeOn(remote_place, fn_dealloc_data, notif_deleteData.getHandle(), nums));
  notif_deleteData.wait();

  pando::deallocateMemory(sum_ptr, 1);
  hb_done.notify();
}

int pandoMain(int argc, char** argv) {
  auto thisPlace = pando::getCurrentPlace();
  bool allowRemoteAccess = getAllowRemoteAccess(argc, argv);
  uint64_t num_hosts = pando::getPlaceDims().node.id;

  if (num_hosts != 2) {
    if (thisPlace.node.id == COORDINATOR_ID)
      std::cerr << "Need 2 hosts for this microbenchmark.\n";
    return 1;
  }

  if (thisPlace.node.id == COORDINATOR_ID) {
    galois::HostLocalStorageHeap::HeapInit();
    galois::PodLocalStorageHeap::HeapInit();

    pando::Notification necessary;
    PANDO_CHECK(necessary.init());
    pando::Place this_place = pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore};
    PANDO_CHECK(pando::executeOn(this_place, &HBMainRemoteAccess, necessary.getHandle(),
                                 allowRemoteAccess));
    necessary.wait();
  }
  pando::waitAll();
  return 0;
}
