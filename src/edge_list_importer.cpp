// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <pando-lib-galois/graphs/edge_list_importer.hpp>
#include <pando-rt/sync/notification.hpp>

pando::Status galois::importELFile(
    std::uint64_t numVertices, const char* filePath,
    pando::GlobalRef<pando::Vector<pando::Vector<std::uint64_t>>> elRef) {
  pando::Status err;
  std::ifstream ifs(filePath, std::ifstream::in);
  if (!ifs.is_open()) {
    return pando::Status::InvalidValue;
  }

  std::uint64_t src{0}, dst{0};

  pando::Vector<pando::Vector<std::uint64_t>> edgeList;
  if ((err = edgeList.initialize(numVertices)) != pando::Status::Success) {
    return err;
  }

  // Construct edge list for each PXN
  for (pando::GlobalRef<pando::Vector<std::uint64_t>> n_edgeListRef : edgeList) {
    pando::Vector<std::uint64_t> n_edgeList;
    if ((err = n_edgeList.initialize(0)) != pando::Status::Success) {
      return err;
    }
    n_edgeListRef = n_edgeList;
  }

  // Move fp to the beginning of the file to read an edge list file again.
  ifs.clear();
  ifs.seekg(0);
  while (ifs >> src && ifs >> dst) {
    // This is copied edgeList[src]'s metadata to srcEdgeList.
    // So, metadata of the original vector instance is not updated
    // even though new elements are pushed.
    // The copied vector instance with the updated information should be
    // copied back to the original vector pointer.
    pando::Vector<std::uint64_t> srcEdgeList = edgeList[src];
    if ((err = srcEdgeList.pushBack(dst)) != pando::Status::Success) {
      return err;
    }
    edgeList[src] = srcEdgeList;
  }
  elRef = edgeList;
  ifs.close();
  return err;
}
