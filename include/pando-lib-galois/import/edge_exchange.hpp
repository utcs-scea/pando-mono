// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_IMPORT_EDGE_EXCHANGE_HPP_
#define PANDO_LIB_GALOIS_IMPORT_EDGE_EXCHANGE_HPP_
#include <ctype.h>
#include <pando-rt/export.h>
#include <stdlib.h>

#include <cassert>
#include <fstream>
#include <iostream>
#include <numeric> // std::iota
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/global_ptr.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/notification.hpp>

struct Edge {
  int64_t src;
  int64_t dest;

  bool operator==(Edge& rhs) {
    return this->src == rhs.src && this->dest == rhs.dest;
  }
};
struct Vertex {
  int64_t global_id;
  int64_t start_indx;
  int64_t num_edges;
};
using EdgeVectorSTL = std::vector<Edge>;
using EdgeVectorPando = pando::Vector<Edge>;
using VertexVectorPando = pando::Vector<Vertex>;
using MetaData = int64_t;

struct GlobalString {
  pando::GlobalPtr<char> str;
  size_t len = 0;
};

GlobalString convertStringToGlobal(std::string input_str);
std::string convertGlobalToString(pando::GlobalPtr<char> global_input);
std::vector<EdgeVectorSTL> get_vHost_edges(std::string input_folder, int64_t num_vhosts);

/*
Hash Vertex_ID into virtualhosts
*/
int64_t hash_vertexID_to_vhost(int64_t vertexID, int64_t num_vhosts);

// KERNEL: Per host
void getVHostData(pando::GlobalPtr<bool> done,
                  pando::GlobalPtr<MetaData> global_vhostMetadataPerHost,
                  pando::GlobalPtr<EdgeVectorPando> global_vhostEdgesPerHost,
                  pando::GlobalPtr<char> input_folder, int64_t num_vhosts_per_host);

// KERNEL: Per host
void read_reduce_local_edge_lists(pando::GlobalPtr<bool> dones,
                                  pando::GlobalPtr<MetaData> global_vhostMetadataPerHost,
                                  pando::GlobalPtr<EdgeVectorPando> global_vhostEdgesPerHost,
                                  pando::GlobalPtr<char> input_folder,
                                  pando::GlobalPtr<MetaData> global_reducedVhostMetadata,
                                  int64_t num_vhosts_per_host);

void sort_metadata(pando::GlobalPtr<MetaData> global_reducedVhostMetadata,
                   pando::GlobalPtr<pando::Vector<int64_t>> sorted_indices_ptr,
                   int64_t num_vhosts_per_host);

// KERNEL: Per host
// Assigns vhosts to hosts in a load-balanced manner
void assign_vhosts_to_host(pando::GlobalPtr<bool> done,
                           pando::GlobalPtr<pando::Vector<int64_t>> vhosts_per_host,
                           pando::GlobalPtr<pando::Vector<int64_t>> sorted_indices_ptr);

void launch_assign_vhosts_to_host(pando::GlobalPtr<bool> dones,
                                  pando::GlobalPtr<pando::Vector<int64_t>> sorted_indices_ptr,
                                  pando::GlobalPtr<pando::Vector<int64_t>> vhosts_per_host);

// KERNEL: Per host
void build_edges_to_send(
    pando::GlobalPtr<bool> done,
    pando::GlobalPtr<pando::Vector<pando::Vector<EdgeVectorPando>>> edges_to_send,
    pando::GlobalPtr<pando::Vector<int64_t>> vhosts_per_host,
    pando::GlobalPtr<EdgeVectorPando> global_vhostEdgesPerHost, int64_t num_vhosts_per_host);

void launch_build_edges_to_send(
    pando::GlobalPtr<bool> dones, pando::GlobalPtr<pando::Vector<int64_t>> vhosts_per_host,
    pando::GlobalPtr<pando::Vector<pando::Vector<EdgeVectorPando>>> edges_to_send,
    pando::GlobalPtr<EdgeVectorPando> global_vhostEdgesPerHost, int64_t num_vhosts_per_host);

// KERNEL: Per host
void edge_exchange(pando::GlobalPtr<bool> done,
                   pando::GlobalPtr<EdgeVectorPando> final_edgelist_per_host,
                   pando::GlobalPtr<pando::Vector<pando::Vector<EdgeVectorPando>>> edges_to_send);

void launch_edge_exchange(
    pando::GlobalPtr<bool> dones, pando::GlobalPtr<EdgeVectorPando> final_edgelist_per_host,
    pando::GlobalPtr<pando::Vector<pando::Vector<EdgeVectorPando>>> edges_to_send);

#endif // PANDO_LIB_GALOIS_IMPORT_EDGE_EXCHANGE_HPP_
