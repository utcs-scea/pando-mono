// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <utils.hpp>

// T = remote ref, F = executeOn
bool getAllowRemoteAccess(int argc, char** argv) {
  // Other libraries may have called getopt before, so we reset optind for correctness
  optind = 0;

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
        return nullptr;
      default:
        std::cerr << "Use -y to allow remote accesses (move-data-to-compute). Default: "
                     "move-compute-to-data\n";
        std::exit(1);
    }
  }

  return allowRemoteAccess;
}

void HBMainRemoteAccess(pando::Notification::HandleType hb_done, bool allowRemoteAccess) {
  // Put data on HOST 1
  galois::Array<uint64_t> nums;
  auto fn_put_data = +[](pando::Notification::HandleType hb_done, galois::Array<uint64_t> nums) {
    uint64_t SIZE = 1000;
    PANDO_CHECK(nums.initialize(SIZE));
    for (uint64_t i = 0; i < SIZE; i++)
      nums[i] = i;
    hb_done.notify();
  };
  pando::Notification notif_initData;
  PANDO_CHECK(notif_initData.init());
  pando::Place remote_place = pando::Place{pando::NodeIndex{1}, pando::anyPod, pando::anyCore};
  PANDO_CHECK(pando::executeOn(remote_place, fn_put_data, notif_initData.getHandle(), nums));
  notif_initData.wait();

  // Sum data on HOST 1

  // De-init data on HOST 1
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
    PANDO_CHECK(pando::executeOn(this_place, &HBMainRemoteAccess, necessary.getHandle()));
    necessary.wait();
  }
  pando::waitAll();
  return 0;
}
