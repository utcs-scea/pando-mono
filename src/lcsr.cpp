// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <pando-lib-galois/graphs/lcsr.hpp>

HashMap<int64_t> create_vhost2host_map(pando::GlobalPtr<pando::Vector<int64_t>> vhosts_per_host,
                                       size_t num_buckets) {
  HashMap<int64_t> vhost2host_map = HashMap<int64_t>();
  vhost2host_map.initialize(num_buckets);
  auto num_hosts = pando::getPlaceDims().node.id;
  for (auto h = 0; h < num_hosts; h++) {
    pando::Vector<int64_t> vhosts = vhosts_per_host[h];
    for (size_t v = 0; v < vhosts.size(); v++) {
      vhost2host_map.insert(vhosts[v], h);
    }
  }
  return vhost2host_map;
}

// Per Host Helper
AdjacencyList create_adjList(EdgeVectorPando edge_list, size_t num_buckets) {
  AdjacencyList adj_list = AdjacencyList();
  adj_list.initialize(num_buckets);
  for (size_t i = 0; i < edge_list.size(); i++) {
    Edge e = edge_list[i];
    adj_list.insert_edge(e);
  }
  return adj_list;
}

// Per Host
void create_local_csr(pando::GlobalPtr<bool> done, pando::GlobalPtr<LocalCSR> hosts_csrs,
                      pando::GlobalPtr<EdgeVectorPando> final_edgelist_per_host,
                      int64_t num_buckets) {
  auto my_host_id = pando::getCurrentPlace().node.id;
  EdgeVectorPando my_edges = final_edgelist_per_host[my_host_id];
  AdjacencyList adj_list = create_adjList(my_edges, num_buckets);
  LocalCSR lcsr = adj_list.get_LocalCSR();
  hosts_csrs[my_host_id] = lcsr;
  *done = true;
}

void build_distGraph(pando::GlobalPtr<bool> dones,
                     pando::GlobalPtr<pando::Vector<int64_t>> vhosts_per_host,
                     pando::GlobalPtr<GlobalGraph> ggraph_ptr,
                     pando::GlobalPtr<EdgeVectorPando> final_edgelist_per_host,
                     int64_t num_vhosts_per_host, int64_t num_buckets) {
  auto num_hosts = pando::getPlaceDims().node.id;
  // Coor builds vhost2host map
  HashMap<int64_t> vhost2host_map = create_vhost2host_map(vhosts_per_host, num_buckets);

  // Makes local CSR and sends ptrs to GP. Creates mapping. Sends info to GP
  // Create GGRAPH:
  GlobalGraph ggraph = GlobalGraph();
  ggraph.initialize(num_vhosts_per_host);
  ggraph.vhost_to_host = vhost2host_map;
  *ggraph_ptr = ggraph;
  for (int16_t i = 0; i < num_hosts; i++) {
    PANDO_CHECK(executeOn(pando::Place{pando::NodeIndex{i}, pando::anyPod, pando::anyCore},
                          &create_local_csr, &dones[i], ggraph.hosts_csrs, final_edgelist_per_host,
                          num_buckets));
  }
  for (int16_t i = 0; i < num_hosts; i++) {
    pando::waitUntil([dones, i]() {
      return dones[i];
    });
  }

  // Reset dones
  for (int16_t i = 0; i < num_hosts; i++)
    dones[i] = false;
}
