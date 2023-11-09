// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
#include <pando-lib-galois/graphs/edge_list_importer.hpp>
#include <pando-rt/sync/notification.hpp>

pando::Status galois::importNaiveCSRFromEdgeListFileOnCPU(
    std::uint64_t numNodes, char* filePath,
    pando::GlobalRef<pando::Vector<pando::Vector<std::uint64_t>>> retRef) {
  pando::Status err;
  std::ifstream ifs(filePath, std::ifstream::in);
  if (!ifs.is_open()) {
    return pando::Status::InvalidValue;
  }
  uint64_t src;
  uint64_t dst;

  pando::Vector<pando::Vector<std::uint64_t>> ret;
  err = ret.initialize(numNodes);

  if (err != pando::Status::Success) {
    return err;
  }

  for (pando::GlobalRef<pando::Vector<std::uint64_t>> vecRef : ret) {
    pando::Vector<std::uint64_t> vec;
    err = vec.initialize(0);
    if (err != pando::Status::Success) {
      return err;
    }
    vecRef = vec;
  }
  while (ifs >> src && ifs >> dst) {
    if (src >= numNodes || dst >= numNodes) {
      continue;
    }
    pando::Vector<std::uint64_t> vec = ret[src];
    err = vec.pushBack(dst);
    if (err != pando::Status::Success) {
      return err;
    }
    ret[src] = vec;
  }
  retRef = ret;
  ifs.close();
  return err;
}
