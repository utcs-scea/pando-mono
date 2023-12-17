// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_GRAPHS_LCSR_HPP_
#define PANDO_LIB_GALOIS_GRAPHS_LCSR_HPP_

#include <utility>

#include "hashmap.hpp"
#include <pando-lib-galois/import/edge_exchange.hpp>

using EdgeVectorPando = pando::Vector<Edge>;
using VertexVectorPando = pando::Vector<Vertex>;
struct LocalCSR {
  VertexVectorPando vertex_csr;
  EdgeVectorPando edge_csr;
  HashMap<int64_t> gid_to_localId;

  void initialize(size_t num_buckets) {
    PANDO_CHECK(vertex_csr.initialize(0));
    PANDO_CHECK(edge_csr.initialize(0));
    gid_to_localId.initialize(num_buckets);
  }

  void deinitialize() {
    vertex_csr.deinitialize();
    edge_csr.deinitialize();
    gid_to_localId.deinitialize();
  }

  bool exists_edge(int64_t gID_src, int64_t gID_dst) {
    Optional<int64_t> op_val = gid_to_localId.lookup(gID_src);
    if (op_val.is_valid) {
      int64_t local_indx = op_val.item;
      Vertex v = vertex_csr[local_indx];
      for (auto i = 0; i < v.num_edges; i++) {
        Edge e = edge_csr[i + v.start_indx];
        if (e.src == gID_src && e.dest == gID_dst)
          return true;
      }
    }
    return false;
  }
};

struct AdjacencyList {
  using DestinationIDVec = pando::Vector<int64_t>;
  HashMap<DestinationIDVec*> adj_list;
  size_t num_buckets = 0;

  void initialize(size_t num_b) {
    num_buckets = num_b;
    adj_list = HashMap<DestinationIDVec*>();
    adj_list.initialize(num_buckets);
  }

  void deinitialize() {
    /*
    Delete all Vector Ptrs
    */
    for (size_t i = 0; i < adj_list.num_buckets; i++) {
      pando::Vector<KVPair<DestinationIDVec*>> bucket = adj_list.buckets_ptr[i];
      for (size_t j = 0; j < bucket.size(); j++) {
        KVPair<DestinationIDVec*> pair = bucket[j];
        delete[] pair.value;
      }
    }

    // De-init global memory
    adj_list.deinitialize();
  }

  void insert_edge(Edge e) {
    // Check src exist?
    Optional<DestinationIDVec*> op_vec_dsts = adj_list.lookup(e.src);
    DestinationIDVec* dsts_ptr = op_vec_dsts.is_valid ? op_vec_dsts.item : new DestinationIDVec();

    // Push back dst on vector
    DestinationIDVec vec_dsts = *dsts_ptr;
    if (!op_vec_dsts.is_valid)
      PANDO_CHECK(vec_dsts.initialize(0));
    PANDO_CHECK(vec_dsts.pushBack(e.dest));
    *dsts_ptr = std::move(vec_dsts);

    // If prev didn't exist, insert into HM
    if (!op_vec_dsts.is_valid)
      adj_list.insert(e.src, dsts_ptr);
  }

  LocalCSR get_LocalCSR() {
    LocalCSR lcsr = LocalCSR();
    lcsr.initialize(num_buckets);
    int64_t start_indx = 0;
    for (size_t i = 0; i < adj_list.num_buckets; i++) {
      pando::Vector<KVPair<DestinationIDVec*>> bucket = adj_list.buckets_ptr[i];
      for (size_t j = 0; j < bucket.size(); j++) {
        KVPair<DestinationIDVec*> pair = bucket[j];

        int64_t src_globalID = pair.key;
        DestinationIDVec* dsts_ptr = pair.value;
        if (dsts_ptr) {
          DestinationIDVec vec_dsts = *dsts_ptr;
          Vertex src_vertex = Vertex{src_globalID, start_indx, (int64_t)vec_dsts.size()};

          // Add Vertex to VertexCSR
          int64_t src_localID = lcsr.vertex_csr.size();
          PANDO_CHECK(lcsr.vertex_csr.pushBack(src_vertex));

          // Add local-global mapping
          lcsr.gid_to_localId.insert(src_globalID, src_localID);

          // Add Edges to EdgeCSR
          for (size_t k = 0; k < vec_dsts.size(); k++) {
            int64_t dst_globalID = vec_dsts[k];
            PANDO_CHECK(lcsr.edge_csr.pushBack(Edge{src_globalID, dst_globalID}));
            start_indx += 1;
          }
        }
      }
    }
    return lcsr;
  }
};

struct GlobalGraph {
  pando::GlobalPtr<LocalCSR> hosts_csrs;
  HashMap<int64_t> vhost_to_host;
  int64_t num_hosts = pando::getPlaceDims().node.id;
  int64_t num_vhosts = 0;

  void initialize(int64_t num_vh) {
    this->num_vhosts = num_vh;
    hosts_csrs = static_cast<pando::GlobalPtr<LocalCSR>>(
        pando::getDefaultMainMemoryResource()->allocate(sizeof(LocalCSR) * num_hosts));
  }

  void deinitialize() {
    for (auto i = 0; i < num_hosts; i++) {
      LocalCSR lcsr = hosts_csrs[i];
      lcsr.deinitialize();
      hosts_csrs[i] = std::move(lcsr);
    }
    if (hosts_csrs != nullptr)
      pando::deallocateMemory(hosts_csrs, num_hosts);
    hosts_csrs = nullptr;
  }

  bool exists_edge(int64_t gID_src, int64_t gID_dst) {
    int64_t vhost = hash_vertexID_to_vhost(gID_src, this->num_vhosts);
    Optional<int64_t> op_host = vhost_to_host.lookup(vhost);
    if (op_host.is_valid) {
      int64_t host = op_host.item;
      LocalCSR host_lcsr = hosts_csrs[host];
      return host_lcsr.exists_edge(gID_src, gID_dst);
    }
    return false;
  }
};

HashMap<int64_t> create_vhost2host_map(pando::GlobalPtr<pando::Vector<int64_t>> vhosts_per_host);

// Per Host Helper
AdjacencyList create_adjList(EdgeVectorPando edge_list);

// Per Host
void create_local_csr(pando::GlobalPtr<bool> done, pando::GlobalPtr<LocalCSR> hosts_csrs,
                      pando::GlobalPtr<EdgeVectorPando> final_edgelist_per_host,
                      int64_t num_buckets);

void build_distGraph(pando::GlobalPtr<bool> dones,
                     pando::GlobalPtr<pando::Vector<int64_t>> vhosts_per_host,
                     pando::GlobalPtr<GlobalGraph> ggraph_ptr,
                     pando::GlobalPtr<EdgeVectorPando> final_edgelist_per_host,
                     int64_t num_vhosts_per_host, int64_t num_buckets);

#endif // PANDO_LIB_GALOIS_GRAPHS_LCSR_HPP_
