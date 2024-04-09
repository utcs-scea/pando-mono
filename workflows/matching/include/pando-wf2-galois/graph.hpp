// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF2_GALOIS_GRAPH_HPP_
#define PANDO_WF2_GALOIS_GRAPH_HPP_

#include "pando-lib-galois/graphs/dist_array_csr.hpp"
#include "pando-rt/containers/vector.hpp"
#include "pando-rt/export.h"
#include "pando-rt/pando-rt.hpp"

#include "WMDParser.hpp"

namespace wf {
/*template <class V, class E>
class Graph {
private:
  pando::GlobalPtr<galois::DistArrayCSR<V, E>> graph_ptr;
public:
  void initialize(std::string file_name) {
    pando::Vector<pando::Vector<std::uint64_t>> edge_list;
    pando::Status err;
    err = edge_list.initialize(0);
    if (err != pando::Status::Success) {
      std::cout << "ERROR\n";
    }
    load_data(file_name, edge_list);

    galois::DistArrayCSR<V, E> graph;
    graph.initialize(edge_list);
    graph_ptr = &graph;
    std::cout << "Initialized graph\n";
    }
  virtual void load_data(std::string file_name, pando::Vector<pando::Vector<uint64_t>> &edge_list) =
0;

};
*/
}; // namespace wf

#endif // PANDO_WF2_GALOIS_GRAPH_HPP_
