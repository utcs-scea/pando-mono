// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <pando-lib-galois/import/edge_exchange.hpp>

GlobalString convertStringToGlobal(std::string input_str) {
  size_t string_len = input_str.size() + 1;
  pando::GlobalPtr<char> global_str = static_cast<pando::GlobalPtr<char>>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(char) * string_len));
  for (size_t i = 0; i < input_str.size(); i++)
    global_str[i] = input_str.c_str()[i];
  global_str[input_str.size()] = '\0';
  return GlobalString{global_str, string_len};
}

std::string convertGlobalToString(pando::GlobalPtr<char> global_input) {
  std::string input_str = "";
  pando::GlobalPtr<char> curr_ptr = global_input;
  while (*curr_ptr != '\0') {
    input_str += *curr_ptr;
    curr_ptr++;
  }
  return std::string(input_str);
}

int64_t hash_vertexID_to_vhost(int64_t vertexID, int64_t num_vhosts) {
  return vertexID % num_vhosts;
}

/*
Create Edges, bucketed according to vhost
*/
std::vector<EdgeVectorSTL> get_vHost_edges(std::string input_folder, int64_t num_vhosts) {
  std::string input_path =
      input_folder + "/" + std::to_string(pando::getCurrentPlace().node.id) + ".csv";
  std::ifstream input_file;
  input_file.open(input_path);

  std::vector<EdgeVectorSTL> vhost_edges(num_vhosts, EdgeVectorSTL());
  std::unordered_map<int64_t, std::set<int64_t>> unique_dests;
  if (input_file) {
    int64_t src = 0;
    int64_t dest = 0;
    while (input_file >> src >> dest) {
      // Assume Edges bi-directional:
      int64_t min_gid = src <= dest ? src : dest;
      int64_t max_gid = src <= dest ? dest : src;

      if (min_gid < max_gid) { // Ensure no self-loops!
        if (unique_dests.find(min_gid) == unique_dests.end())
          unique_dests[min_gid] = std::set<int64_t>();
        if (unique_dests[min_gid].find(max_gid) == unique_dests[min_gid].end()) {
          unique_dests[min_gid].insert(max_gid);

          // Hash vertexID
          auto vhost_indx = hash_vertexID_to_vhost(min_gid, num_vhosts);

          // Put edge in bucket
          vhost_edges[vhost_indx].push_back(Edge{min_gid, max_gid});
        }
      }
    }
    input_file.close();
  } else {
    std::cerr << input_path << " NOT FOUND!" << std::endl;
    assert(false);
  }

  return vhost_edges;
}

void getVHostData(pando::GlobalPtr<bool> done,
                  pando::GlobalPtr<MetaData> global_vhostMetadataPerHost,
                  pando::GlobalPtr<EdgeVectorPando> global_vhostEdgesPerHost,
                  pando::GlobalPtr<char> input_folder, int64_t num_vhosts_per_host) {
  std::vector<EdgeVectorSTL> vhost_edges =
      get_vHost_edges(convertGlobalToString(input_folder), num_vhosts_per_host);

  for (auto i = 0; i < num_vhosts_per_host; i++) {
    auto numEdges = vhost_edges[i].size();
    global_vhostMetadataPerHost[i] = numEdges;

    // Alloc edges locally and put somewhere other hosts can see.
    EdgeVectorPando tvec = global_vhostEdgesPerHost[i];
    PANDO_CHECK(tvec.initialize(numEdges));
    for (int64_t j = 0; j < (int64_t)numEdges; j++)
      tvec[j] = vhost_edges[i][j];
    global_vhostEdgesPerHost[i] = std::move(tvec);
  }

  *done = true;
}

void read_reduce_local_edge_lists(pando::GlobalPtr<bool> dones,
                                  pando::GlobalPtr<MetaData> global_vhostMetadataPerHost,
                                  pando::GlobalPtr<EdgeVectorPando> global_vhostEdgesPerHost,
                                  pando::GlobalPtr<char> input_folder,
                                  pando::GlobalPtr<MetaData> global_reducedVhostMetadata,
                                  int64_t num_vhosts_per_host) {
  auto num_hosts = pando::getPlaceDims().node.id;

  // Reads each hosts' data into local Edge List. Collects Metadata. Assigns each edge to VHOST
  for (int64_t i = 0; i < num_hosts; i++) {
    auto vhost_index = i * num_vhosts_per_host;
    PANDO_CHECK(executeOn(pando::Place{pando::NodeIndex{i}, pando::anyPod, pando::anyCore},
                          &getVHostData, &dones[i], &global_vhostMetadataPerHost[vhost_index],
                          &global_vhostEdgesPerHost[vhost_index], input_folder,
                          num_vhosts_per_host));
  }

  // REDUCE Metadata:
  for (int64_t i = 0; i < num_hosts; i++) {
    pando::waitUntil([dones, i]() {
      return dones[i];
    });
    auto vhost_index = i * num_vhosts_per_host;
    for (auto j = 0; j < num_vhosts_per_host; j++) {
      global_reducedVhostMetadata[j] += global_vhostMetadataPerHost[vhost_index + j];
    }
  }

  // RESET dones:
  for (int64_t i = 0; i < num_hosts; i++)
    dones[i] = false;
}

void sort_metadata(pando::GlobalPtr<MetaData> global_reducedVhostMetadata,
                   pando::GlobalPtr<pando::Vector<int64_t>> sorted_indices_ptr,
                   int64_t num_vhosts_per_host) {
  // Copy Metadata into vector
  std::vector<MetaData> metadata_to_sort(num_vhosts_per_host);
  for (auto i = 0; i < num_vhosts_per_host; i++)
    metadata_to_sort[i] = global_reducedVhostMetadata[i];

  // Sort Metadata
  std::vector<int64_t> vector_indices(num_vhosts_per_host);
  std::iota(vector_indices.begin(), vector_indices.end(), 0);
  std::sort(vector_indices.begin(), vector_indices.end(), [&](MetaData i, MetaData j) {
    return metadata_to_sort[i] < metadata_to_sort[j];
  });

  // Copy sorted info into GlobalPtr
  pando::Vector<int64_t> sorted_indices = *sorted_indices_ptr;
  size_t sz_to_init = metadata_to_sort.size();
  PANDO_CHECK(sorted_indices.initialize(sz_to_init));
  for (size_t i = 0; i < metadata_to_sort.size(); i++)
    sorted_indices[i] = vector_indices[i];
  *sorted_indices_ptr = std::move(sorted_indices);
}

void assign_vhosts_to_host(pando::GlobalPtr<bool> done,
                           pando::GlobalPtr<pando::Vector<int64_t>> vhosts_per_host,
                           pando::GlobalPtr<pando::Vector<int64_t>> sorted_indices_ptr) {
  auto my_host_id = pando::getCurrentPlace().node.id;
  auto num_hosts = pando::getPlaceDims().node.id;
  pando::Vector<int64_t> sorted_indices = *sorted_indices_ptr;
  pando::Vector<int64_t> my_assigned_vhosts = vhosts_per_host[my_host_id];
  PANDO_CHECK(my_assigned_vhosts.initialize(0));
  for (size_t i = my_host_id; i < sorted_indices.size(); i += num_hosts) {
    int64_t vhost = sorted_indices[i];
    PANDO_CHECK(my_assigned_vhosts.pushBack(vhost));
  }

  vhosts_per_host[my_host_id] = std::move(my_assigned_vhosts);
  *done = true;
}

void launch_assign_vhosts_to_host(pando::GlobalPtr<bool> dones,
                                  pando::GlobalPtr<pando::Vector<int64_t>> sorted_indices_ptr,
                                  pando::GlobalPtr<pando::Vector<int64_t>> vhosts_per_host) {
  auto num_hosts = pando::getPlaceDims().node.id;

  // LAUNCH KERNEL: Build mapping
  for (int64_t i = 0; i < num_hosts; i++) {
    PANDO_CHECK(executeOn(pando::Place{pando::NodeIndex{i}, pando::anyPod, pando::anyCore},
                          &assign_vhosts_to_host, &dones[i], vhosts_per_host, sorted_indices_ptr));
  }
  for (int64_t i = 0; i < num_hosts; i++) {
    pando::waitUntil([dones, i]() {
      return dones[i];
    });
  }

  // Reset dones
  for (int64_t i = 0; i < num_hosts; i++)
    dones[i] = false;
}

void build_edges_to_send(
    pando::GlobalPtr<bool> done,
    pando::GlobalPtr<pando::Vector<pando::Vector<EdgeVectorPando>>> edges_to_send,
    pando::GlobalPtr<pando::Vector<int64_t>> vhosts_per_host,
    pando::GlobalPtr<EdgeVectorPando> global_vhostEdgesPerHost, int64_t num_vhosts_per_host) {
  auto my_host_id = pando::getCurrentPlace().node.id;
  auto num_hosts = pando::getPlaceDims().node.id;
  pando::Vector<pando::Vector<EdgeVectorPando>> destination_buffer = edges_to_send[my_host_id];
  PANDO_CHECK(destination_buffer.initialize(static_cast<size_t>(num_hosts)));
  for (int64_t i = 0; i < num_hosts; i++) {
    // Get all vhosts for host i
    pando::Vector<int64_t> vhosts_hostI = vhosts_per_host[i];
    pando::Vector<EdgeVectorPando> vhostEdgeListsForHostI = destination_buffer[i];
    PANDO_CHECK(vhostEdgeListsForHostI.initialize(vhosts_hostI.size()));

    for (size_t j = 0; j < vhosts_hostI.size(); j++) {
      int64_t vhost = vhosts_hostI[j];

      // Append EV_vhostj_hostI
      int64_t flat_indx = (my_host_id * num_vhosts_per_host) + vhost;
      EdgeVectorPando edge_vector = global_vhostEdgesPerHost[flat_indx];
      vhostEdgeListsForHostI[j] = std::move(edge_vector);
    }

    destination_buffer[i] = std::move(vhostEdgeListsForHostI);
  }

  edges_to_send[my_host_id] = std::move(destination_buffer);
  *done = true;
}

void launch_build_edges_to_send(
    pando::GlobalPtr<bool> dones, pando::GlobalPtr<pando::Vector<int64_t>> vhosts_per_host,
    pando::GlobalPtr<pando::Vector<pando::Vector<EdgeVectorPando>>> edges_to_send,
    pando::GlobalPtr<EdgeVectorPando> global_vhostEdgesPerHost, int64_t num_vhosts_per_host) {
  auto num_hosts = pando::getPlaceDims().node.id;

  // LAUNCH KERNEL:: build_edges_to_send
  for (int64_t i = 0; i < num_hosts; i++) {
    PANDO_CHECK(executeOn(pando::Place{pando::NodeIndex{i}, pando::anyPod, pando::anyCore},
                          &build_edges_to_send, &dones[i], edges_to_send, vhosts_per_host,
                          global_vhostEdgesPerHost, num_vhosts_per_host));
  }
  for (int64_t i = 0; i < num_hosts; i++)
    pando::waitUntil([dones, i]() {
      return dones[i];
    });

  // Reset dones
  for (int64_t i = 0; i < num_hosts; i++)
    dones[i] = false;
}

// KERNEL: Per host
void edge_exchange(pando::GlobalPtr<bool> done,
                   pando::GlobalPtr<EdgeVectorPando> final_edgelist_per_host,
                   pando::GlobalPtr<pando::Vector<pando::Vector<EdgeVectorPando>>> edges_to_send) {
  auto my_host_id = pando::getCurrentPlace().node.id;
  auto num_hosts = pando::getPlaceDims().node.id;
  EdgeVectorPando my_edges = final_edgelist_per_host[my_host_id];
  PANDO_CHECK(my_edges.initialize(0));
  for (int64_t h = 0; h < num_hosts; h++) {
    pando::Vector<pando::Vector<EdgeVectorPando>> allVHostEdges = edges_to_send[h];
    pando::Vector<EdgeVectorPando> vhostEdgesFound = allVHostEdges[my_host_id];
    for (size_t i = 0; i < vhostEdgesFound.size(); i++) {
      EdgeVectorPando src_edges = vhostEdgesFound[i];
      for (size_t j = 0; j < src_edges.size(); j++)
        PANDO_CHECK(my_edges.pushBack(src_edges[j]));
    }
  }

  final_edgelist_per_host[my_host_id] = std::move(my_edges);
  *done = true;
}

void launch_edge_exchange(
    pando::GlobalPtr<bool> dones, pando::GlobalPtr<EdgeVectorPando> final_edgelist_per_host,
    pando::GlobalPtr<pando::Vector<pando::Vector<EdgeVectorPando>>> edges_to_send) {
  auto num_hosts = pando::getPlaceDims().node.id;

  // LAUNCH KERNEL:: edgeExchange
  for (int64_t i = 0; i < num_hosts; i++) {
    PANDO_CHECK(executeOn(pando::Place{pando::NodeIndex{i}, pando::anyPod, pando::anyCore},
                          &edge_exchange, &dones[i], final_edgelist_per_host, edges_to_send));
  }
  for (int64_t i = 0; i < num_hosts; i++)
    pando::waitUntil([dones, i]() {
      return dones[i];
    });

  // Reset dones
  for (int64_t i = 0; i < num_hosts; i++)
    dones[i] = false;
}
