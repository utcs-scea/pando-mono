// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.
#include "pando-rt/export.h"
#include "pando-rt/pando-rt.hpp"
#include "pando-wf2-galois/export.h"
#include "pando-wf2-galois/import_wmd.hpp"
#include "pando-wf2-galois/partial_pattern.h"

int pandoMain(int argc, char** argv) {
  if (pando::getCurrentPlace().node.id == 0) {
    galois::HostLocalStorageHeap::HeapInit();
    galois::PodLocalStorageHeap::HeapInit();
  }
  if (argc != 2) {
    PANDO_ABORT("Graph name expected as an argument\n");
  }
  std::string filename(argv[1]);

  auto place = pando::getCurrentPlace();

  // Import Graph
  if (place.node.id == 0) {
    pando::GlobalPtr<wf2::partial::Graph> graph_ptr = wf::ImportWMDGraph(filename);

    wf2::partial::partialMatch(graph_ptr);
  }

  pando::waitAll();
  return 0;
}
