// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.
#ifndef PANDO_WF2_GALOIS_IMPORT_WMD_HPP_
#define PANDO_WF2_GALOIS_IMPORT_WMD_HPP_

#include <fstream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "graph_ds.h"
#include "pando-lib-galois/graphs/dist_array_csr.hpp"
#include "pando-rt/export.h"
#include "pando-rt/pando-rt.hpp"
#include <pando-lib-galois/graphs/dist_local_csr.hpp>
#include <pando-lib-galois/graphs/wmd_graph.hpp>
#include <pando-lib-galois/import/wmd_graph_importer.hpp>
#include <pando-lib-galois/utility/agile_schema.hpp>

#define MEASURE_TIME 0
#define DEBUG_IMPORT 0
#if DEBUG_IMPORT == 0
#define DBG_IMPORT_PRINT(x)
#endif
#if DEBUG_IMPORT == 1
#define DBG_IMPORT_PRINT(x) \
  { x }
#endif

namespace wf {

template <typename V, typename E>
using ArrayGraph = galois::DistArrayCSR<V, E>;
using WMDGraph = galois::DistLocalCSR<wf::WMDVertex, wf::WMDEdge>;
template <typename V, typename E>
using Graph = ArrayGraph<V, E>;

pando::GlobalPtr<WMDGraph> ImportWMDGraph(std::string filename) {
  pando::Array<char> filename_arr;
  PANDO_CHECK(filename_arr.initialize(filename.size()));
  for (uint64_t i = 0; i < filename.size(); i++)
    filename_arr[i] = filename[i];

  WMDGraph graph;
  pando::GlobalPtr<WMDGraph> graph_ptr = static_cast<decltype(graph_ptr)>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(WMDGraph)));
  // bool isEdgeList = false;
  // PANDO_CHECK(graph_ptr->initializeWMD(filename_arr, isEdgeList));

  return graph_ptr;
}

}; // namespace wf

#endif // PANDO_WF2_GALOIS_IMPORT_WMD_HPP_
