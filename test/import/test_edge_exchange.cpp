// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <pando-lib-galois/import/edge_exchange.hpp>

#define NUM_VHOSTS_PER_HOST 8

void check_dones_reset(pando::GlobalPtr<bool> dones) {
  auto num_hosts = pando::getPlaceDims().node.id;
  for (int64_t i = 0; i < num_hosts; i++)
    EXPECT_EQ(dones[i], false);
}

void test_read_reduce_local_edge_lists(std::vector<EdgeVectorSTL> expected_vhostEdgesPerHost,
                                       std::vector<MetaData> expected_reducedVhostMetadata,
                                       std::string given_input_folder) {
  // Initialize State
  auto num_hosts = pando::getPlaceDims().node.id;
  auto size = NUM_VHOSTS_PER_HOST * num_hosts;

  GlobalString input_folder_str = convertStringToGlobal(given_input_folder);
  pando::GlobalPtr<bool> dones = static_cast<pando::GlobalPtr<bool>>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(bool) * num_hosts));
  pando::GlobalPtr<EdgeVectorPando> global_vhostEdgesPerHost =
      static_cast<pando::GlobalPtr<EdgeVectorPando>>(
          pando::getDefaultMainMemoryResource()->allocate(sizeof(EdgeVectorPando) * size));
  pando::GlobalPtr<MetaData> global_vhostMetadataPerHost = static_cast<pando::GlobalPtr<MetaData>>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(MetaData) * size));
  pando::GlobalPtr<MetaData> global_reducedVhostMetadata = static_cast<pando::GlobalPtr<MetaData>>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(MetaData) * NUM_VHOSTS_PER_HOST));

  read_reduce_local_edge_lists(dones, global_vhostMetadataPerHost, global_vhostEdgesPerHost,
                               input_folder_str.str, global_reducedVhostMetadata,
                               NUM_VHOSTS_PER_HOST);

  // ---------------- Check Final State ----------------
  check_dones_reset(dones);
  // Check correct metadata
  EXPECT_EQ(NUM_VHOSTS_PER_HOST, expected_reducedVhostMetadata.size());
  for (auto i = 0; i < NUM_VHOSTS_PER_HOST; i++)
    EXPECT_EQ(expected_reducedVhostMetadata[i], global_reducedVhostMetadata[i]);

  // Check read in all edges into correct locations
  for (auto i = 0; i < size; i++) {
    EdgeVectorPando actual_edges_vhost_host = global_vhostEdgesPerHost[i];
    EdgeVectorSTL expected_edges_vhost_host = expected_vhostEdgesPerHost[i];
    EXPECT_EQ(actual_edges_vhost_host.size(), expected_edges_vhost_host.size());

    for (size_t j = 0; j < actual_edges_vhost_host.size(); j++) {
      Edge actual_e = actual_edges_vhost_host[j];
      bool found_edge = expected_edges_vhost_host.end() !=
                        std::find_if(expected_edges_vhost_host.begin(),
                                     expected_edges_vhost_host.end(), [&actual_e](Edge& e) -> bool {
                                       return actual_e == e;
                                     });
      EXPECT_EQ(found_edge, true);
    }
  }

  // Free State
  pando::deallocateMemory(input_folder_str.str, input_folder_str.len);
  pando::deallocateMemory(dones, num_hosts);
  pando::deallocateMemory(global_vhostMetadataPerHost, size);
  pando::deallocateMemory(global_reducedVhostMetadata, NUM_VHOSTS_PER_HOST);
}

void run_test_read_reduce_local_edge_lists(pando::Notification::HandleType hb_done) {
  std::vector<EdgeVectorSTL> expected_vhostEdgesPerHost = {
      {Edge{8, 9}},             // 0
      {Edge{1, 2}, Edge{1, 3}}, // 1
      {},                       // 2
      {Edge{3, 4}},             // 3
      {},                       // 4
      {},                       // 5
      {},                       // 6
      {},                       // 7
      {},                       // 8
      {},                       // 9
      {},                       // 10
      {},                       // 11
      {Edge{4, 5}, Edge{4, 6}}, // 12
      {Edge{5, 6}},             // 13
      {Edge{6, 7}},             // 14
      {},                       // 15
      {},                       // 16
      {Edge{1, 7}},             // 17
      {Edge{2, 3}, Edge{2, 7}}, // 18
      {},                       // 19
      {},                       // 20
      {},                       // 21
      {},                       // 22
      {Edge{7, 8}},             // 23
  };
  std::vector<MetaData> expected_reducedVhostMetadata = {1, 3, 2, 1, 2, 1, 1, 1};
  test_read_reduce_local_edge_lists(expected_vhostEdgesPerHost, expected_reducedVhostMetadata,
                                    "graphs/graph_csvs/simple_el");
  hb_done.notify();
}

void test_sort_metadata(std::vector<int64_t> expected_sorted_indices,
                        pando::GlobalPtr<MetaData> global_reducedVhostMetadata) {
  // Init State
  pando::GlobalPtr<pando::Vector<int64_t>> sorted_indices_ptr =
      static_cast<pando::GlobalPtr<pando::Vector<int64_t>>>(
          pando::getDefaultMainMemoryResource()->allocate(sizeof(pando::Vector<int64_t>)));

  sort_metadata(global_reducedVhostMetadata, sorted_indices_ptr, NUM_VHOSTS_PER_HOST);
  pando::Vector<int64_t> sorted_indices = *sorted_indices_ptr;
  EXPECT_EQ(sorted_indices.size(), expected_sorted_indices.size());
  for (auto i = 0; i < NUM_VHOSTS_PER_HOST; i++)
    EXPECT_EQ(sorted_indices[i], expected_sorted_indices[i]);

  // Free State
  pando::deallocateMemory(sorted_indices_ptr, 1);
}

void run_test_sort_metadata(pando::Notification::HandleType hb_done) {
  std::vector<int64_t> expected_sorted_indices = {0, 3, 5, 6, 7, 2, 4, 1};
  std::vector<MetaData> given_reducedVhostMetadata = {1, 3, 2, 1, 2, 1, 1, 1};
  pando::GlobalPtr<MetaData> global_reducedVhostMetadata = static_cast<pando::GlobalPtr<MetaData>>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(MetaData) * NUM_VHOSTS_PER_HOST));

  EXPECT_EQ(given_reducedVhostMetadata.size(), NUM_VHOSTS_PER_HOST);
  for (size_t i = 0; i < given_reducedVhostMetadata.size(); i++)
    global_reducedVhostMetadata[i] = given_reducedVhostMetadata[i];
  test_sort_metadata(expected_sorted_indices, global_reducedVhostMetadata);

  pando::deallocateMemory(global_reducedVhostMetadata, NUM_VHOSTS_PER_HOST);
  hb_done.notify();
}

void test_launch_assign_vhosts_to_host(
    std::vector<std::vector<int64_t>> expected_vhosts_per_host,
    pando::GlobalPtr<pando::Vector<int64_t>> sorted_indices_ptr) {
  auto num_hosts = pando::getPlaceDims().node.id;
  pando::GlobalPtr<bool> dones = static_cast<pando::GlobalPtr<bool>>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(bool) * num_hosts));
  pando::GlobalPtr<pando::Vector<int64_t>> vhosts_per_host =
      static_cast<pando::GlobalPtr<pando::Vector<int64_t>>>(
          pando::getDefaultMainMemoryResource()->allocate(sizeof(pando::Vector<int64_t>) *
                                                          num_hosts));

  pando::Vector<int64_t> sorted_indices = *sorted_indices_ptr;
  EXPECT_EQ(sorted_indices.size(), NUM_VHOSTS_PER_HOST);
  launch_assign_vhosts_to_host(dones, sorted_indices_ptr, vhosts_per_host);

  check_dones_reset(dones);
  EXPECT_EQ(expected_vhosts_per_host.size(), num_hosts);
  for (auto i = 0; i < num_hosts; i++) {
    pando::Vector<int64_t> vhosts_host_i = vhosts_per_host[i];
    std::vector<int64_t> expected_vhosts_i = expected_vhosts_per_host[i];
    for (size_t j = 0; j < vhosts_host_i.size(); j++) {
      // Find vhost in expected vhosts
      bool found_vhost =
          expected_vhosts_i.end() !=
          std::find(expected_vhosts_i.begin(), expected_vhosts_i.end(), vhosts_host_i[j]);
      EXPECT_EQ(found_vhost, true);
    }
  }

  pando::deallocateMemory(dones, num_hosts);
  pando::deallocateMemory(vhosts_per_host, num_hosts);
}
void run_test_launch_assign_vhosts_to_host(pando::Notification::HandleType hb_done) {
  std::vector<std::vector<int64_t>> expected_vhosts_per_host = {{0, 6, 4}, {3, 7, 1}, {5, 2}};
  std::vector<int64_t> given_sorted_indices = {0, 3, 5, 6, 7, 2, 4, 1};
  pando::GlobalPtr<pando::Vector<int64_t>> sorted_indices_ptr =
      static_cast<pando::GlobalPtr<pando::Vector<int64_t>>>(
          pando::getDefaultMainMemoryResource()->allocate(sizeof(pando::Vector<int64_t>)));
  pando::Vector<int64_t> sorted_indices = *sorted_indices_ptr;
  PANDO_CHECK(sorted_indices.initialize(given_sorted_indices.size()));
  for (size_t i = 0; i < given_sorted_indices.size(); i++)
    sorted_indices[i] = given_sorted_indices[i];
  *sorted_indices_ptr = std::move(sorted_indices);

  test_launch_assign_vhosts_to_host(expected_vhosts_per_host, sorted_indices_ptr);

  pando::deallocateMemory(sorted_indices_ptr, 1);
  hb_done.notify();
}

void test_launch_build_edges_to_send(
    std::vector<std::vector<std::vector<std::vector<Edge>>>> expected_edges_to_send,
    pando::GlobalPtr<EdgeVectorPando> global_vhostEdgesPerHost,
    pando::GlobalPtr<pando::Vector<int64_t>> vhosts_per_host) {
  auto num_hosts = pando::getPlaceDims().node.id;
  pando::GlobalPtr<bool> dones = static_cast<pando::GlobalPtr<bool>>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(bool) * num_hosts));
  pando::GlobalPtr<pando::Vector<pando::Vector<EdgeVectorPando>>> edges_to_send =
      static_cast<pando::GlobalPtr<pando::Vector<pando::Vector<EdgeVectorPando>>>>(
          pando::getDefaultMainMemoryResource()->allocate(
              sizeof(pando::Vector<pando::Vector<EdgeVectorPando>>) * num_hosts));
  launch_build_edges_to_send(dones, vhosts_per_host, edges_to_send, global_vhostEdgesPerHost,
                             NUM_VHOSTS_PER_HOST);

  // Check Answer
  check_dones_reset(dones);
  EXPECT_EQ(num_hosts, expected_edges_to_send.size());
  for (auto i = 0; i < num_hosts; i++) {
    pando::Vector<pando::Vector<EdgeVectorPando>> check_hosti_edges_to_send = edges_to_send[i];
    EXPECT_EQ(num_hosts, check_hosti_edges_to_send.size());

    for (size_t j = 0; j < check_hosti_edges_to_send.size(); j++) {
      pando::Vector<int64_t> vhosts_hostI = vhosts_per_host[j];
      pando::Vector<EdgeVectorPando> edges_for_hostJ = check_hosti_edges_to_send[j];
      EXPECT_EQ(edges_for_hostJ.size(), vhosts_hostI.size());
      for (size_t k = 0; k < edges_for_hostJ.size(); k++) {
        EdgeVectorPando evK = edges_for_hostJ[k];
        for (size_t l = 0; l < evK.size(); l++) {
          Edge actual_e = evK[l];

          // Find e in expected
          std::vector<Edge> expected_location_of_e = expected_edges_to_send[i][j][k];
          bool found_edge =
              expected_location_of_e.end() != std::find_if(expected_location_of_e.begin(),
                                                           expected_location_of_e.end(),
                                                           [&actual_e](Edge& e) -> bool {
                                                             return actual_e == e;
                                                           });
          EXPECT_EQ(found_edge, true);
        }
      }
    }
  }
  pando::deallocateMemory(dones, num_hosts);
  pando::deallocateMemory(edges_to_send, num_hosts);
}
void run_test_launch_build_edges_to_send(pando::Notification::HandleType hb_done) {
  std::vector<std::vector<std::vector<std::vector<Edge>>>> expected_edges_to_send = {
      {
          // Host 0
          { // Send to Host 0
           {// EV 0
            Edge{8, 9}},
           {},
           {}},
          { // Send to Host 1
           {// EV 0
            Edge{3, 4}},
           {},
           {
               // EV 2
               Edge{1, 2},
               Edge{1, 3},
           }},
          {{}, {}}, // Send to Host 2
      },
      { // HOST 1
       {// Send to host 0
        {},
        {// EV 1
         Edge{6, 7}},
        {// EV 2
         Edge{4, 5}, Edge{4, 6}}},
       {{}, {}, {}}, // Send to host 1
       {
           // Send to host 2
           {Edge{5, 6}},
           {},
       }},
      {              // HOST 2
       {{}, {}, {}}, // Send to host 0
       {             // Send to host 1
        {},
        {Edge{7, 8}},
        {Edge{1, 7}}},
       {// Send to host 2
        {},
        {Edge{2, 3}, Edge{2, 7}}}}};
  auto num_hosts = pando::getPlaceDims().node.id;
  auto size = num_hosts * NUM_VHOSTS_PER_HOST;

  pando::GlobalPtr<pando::Vector<int64_t>> vhosts_per_host =
      static_cast<pando::GlobalPtr<pando::Vector<int64_t>>>(
          pando::getDefaultMainMemoryResource()->allocate(sizeof(pando::Vector<int64_t>) *
                                                          num_hosts));
  pando::GlobalPtr<EdgeVectorPando> global_vhostEdgesPerHost =
      static_cast<pando::GlobalPtr<EdgeVectorPando>>(
          pando::getDefaultMainMemoryResource()->allocate(sizeof(EdgeVectorPando) * size));

  std::vector<EdgeVectorSTL> given_vhostEdgesPerHost = {
      {Edge{8, 9}},             // 0
      {Edge{1, 2}, Edge{1, 3}}, // 1
      {},                       // 2
      {Edge{3, 4}},             // 3
      {},                       // 4
      {},                       // 5
      {},                       // 6
      {},                       // 7
      {},                       // 8
      {},                       // 9
      {},                       // 10
      {},                       // 11
      {Edge{4, 5}, Edge{4, 6}}, // 12
      {Edge{5, 6}},             // 13
      {Edge{6, 7}},             // 14
      {},                       // 15
      {},                       // 16
      {Edge{1, 7}},             // 17
      {Edge{2, 3}, Edge{2, 7}}, // 18
      {},                       // 19
      {},                       // 20
      {},                       // 21
      {},                       // 22
      {Edge{7, 8}},             // 23
  };
  for (auto i = 0; i < size; i++) {
    EdgeVectorPando ev = global_vhostEdgesPerHost[i];
    PANDO_CHECK(ev.initialize(0));
    for (size_t j = 0; j < given_vhostEdgesPerHost[i].size(); j++) {
      PANDO_CHECK(ev.pushBack(given_vhostEdgesPerHost[i][j]));
    }
    global_vhostEdgesPerHost[i] = std::move(ev);
  }
  std::vector<std::vector<int64_t>> given_vhosts_per_host = {{0, 6, 4}, {3, 7, 1}, {5, 2}};
  for (auto i = 0; i < num_hosts; i++) {
    pando::Vector<int64_t> vhosts_host_i = vhosts_per_host[i];
    PANDO_CHECK(vhosts_host_i.initialize(0));
    for (size_t j = 0; j < given_vhosts_per_host[j].size(); j++) {
      PANDO_CHECK(vhosts_host_i.pushBack(given_vhosts_per_host[i][j]));
    }
    vhosts_per_host[i] = std::move(vhosts_host_i);
  }

  test_launch_build_edges_to_send(expected_edges_to_send, global_vhostEdgesPerHost,
                                  vhosts_per_host);

  pando::deallocateMemory(vhosts_per_host, num_hosts);
  pando::deallocateMemory(global_vhostEdgesPerHost, size);
  hb_done.notify();
}

void test_launch_edge_exchange(
    std::vector<EdgeVectorSTL> expected_final_EL,
    pando::GlobalPtr<pando::Vector<pando::Vector<EdgeVectorPando>>> edges_to_send) {
  auto num_hosts = pando::getPlaceDims().node.id;
  pando::GlobalPtr<bool> dones = static_cast<pando::GlobalPtr<bool>>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(bool) * num_hosts));
  pando::GlobalPtr<EdgeVectorPando> final_edgelist_per_host =
      static_cast<pando::GlobalPtr<EdgeVectorPando>>(
          pando::getDefaultMainMemoryResource()->allocate(sizeof(EdgeVectorPando) * num_hosts));

  launch_edge_exchange(dones, final_edgelist_per_host, edges_to_send);
  EXPECT_EQ(num_hosts, expected_final_EL.size());
  for (auto i = 0; i < num_hosts; i++) {
    EdgeVectorPando el_hostI = final_edgelist_per_host[i];
    EdgeVectorSTL expected_el_hostI = expected_final_EL[i];
    EXPECT_EQ(el_hostI.size(), expected_el_hostI.size());
    for (size_t j = 0; j < el_hostI.size(); j++) {
      Edge actual_e = el_hostI[j];
      bool found_edge = expected_el_hostI.end() != std::find_if(expected_el_hostI.begin(),
                                                                expected_el_hostI.end(),
                                                                [&actual_e](Edge& e) -> bool {
                                                                  return actual_e == e;
                                                                });
      EXPECT_EQ(found_edge, true);
    }
  }

  check_dones_reset(dones);
  pando::deallocateMemory(dones, num_hosts);
  pando::deallocateMemory(final_edgelist_per_host, num_hosts);
}
void run_test_launch_edge_exchange(pando::Notification::HandleType hb_done) {
  std::vector<EdgeVectorSTL> expected_final_EL = {
      {Edge{8, 9}, Edge{6, 7}, Edge{4, 5}, Edge{4, 6}},
      {Edge{3, 4}, Edge{1, 2}, Edge{1, 3}, Edge{7, 8}, Edge{1, 7}},
      {Edge{5, 6}, Edge{2, 3}, Edge{2, 7}}};
  auto num_hosts = pando::getPlaceDims().node.id;
  pando::GlobalPtr<pando::Vector<pando::Vector<EdgeVectorPando>>> edges_to_send =
      static_cast<pando::GlobalPtr<pando::Vector<pando::Vector<EdgeVectorPando>>>>(
          pando::getDefaultMainMemoryResource()->allocate(
              sizeof(pando::Vector<pando::Vector<EdgeVectorPando>>) * num_hosts));
  std::vector<std::vector<std::vector<std::vector<Edge>>>> given_edges_to_send = {
      {
          // Host 0
          { // Send to Host 0
           {// EV 0
            Edge{8, 9}},
           {},
           {}},
          { // Send to Host 1
           {// EV 0
            Edge{3, 4}},
           {},
           {
               // EV 2
               Edge{1, 2},
               Edge{1, 3},
           }},
          {{}, {}}, // Send to Host 2
      },
      { // HOST 1
       {// Send to host 0
        {},
        {// EV 1
         Edge{6, 7}},
        {// EV 2
         Edge{4, 5}, Edge{4, 6}}},
       {{}, {}, {}}, // Send to host 1
       {
           // Send to host 2
           {Edge{5, 6}},
           {},
       }},
      {              // HOST 2
       {{}, {}, {}}, // Send to host 0
       {             // Send to host 1
        {},
        {Edge{7, 8}},
        {Edge{1, 7}}},
       {// Send to host 2
        {},
        {Edge{2, 3}, Edge{2, 7}}}}};

  for (auto i = 0; i < num_hosts; i++) {
    pando::Vector<pando::Vector<EdgeVectorPando>> check_hosti_edges_to_send = edges_to_send[i];
    PANDO_CHECK(check_hosti_edges_to_send.initialize(given_edges_to_send.at(i).size()));
    for (size_t j = 0; j < check_hosti_edges_to_send.size(); j++) {
      pando::Vector<EdgeVectorPando> edges_for_hostJ = check_hosti_edges_to_send[j];
      PANDO_CHECK(edges_for_hostJ.initialize(given_edges_to_send.at(i).at(j).size()));

      for (size_t k = 0; k < edges_for_hostJ.size(); k++) {
        EdgeVectorPando evK = edges_for_hostJ[k];
        PANDO_CHECK(evK.initialize(given_edges_to_send.at(i).at(j).at(k).size()));
        for (size_t l = 0; l < evK.size(); l++)
          evK[l] = given_edges_to_send.at(i).at(j).at(k).at(l);
        edges_for_hostJ[k] = std::move(evK);
      }
      check_hosti_edges_to_send[j] = std::move(edges_for_hostJ);
    }
    edges_to_send[i] = std::move(check_hosti_edges_to_send);
  }

  test_launch_edge_exchange(expected_final_EL, edges_to_send);

  pando::deallocateMemory(edges_to_send, num_hosts);
  hb_done.notify();
}

TEST(TriangleCount, SimpleRRLocalELs) {
  int64_t num_hosts = pando::getPlaceDims().node.id;
  int64_t required_num_hosts = 3;
  if (num_hosts == required_num_hosts) {
    pando::Notification necessary;
    PANDO_CHECK(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore},
                                 &run_test_read_reduce_local_edge_lists, necessary.getHandle()));
    necessary.wait();
  }
}

TEST(TriangleCount, SimpleSortMetadata) {
  int64_t num_hosts = pando::getPlaceDims().node.id;
  int64_t required_num_hosts = 3;
  if (num_hosts == required_num_hosts) {
    pando::Notification necessary;
    PANDO_CHECK(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore},
                                 &run_test_sort_metadata, necessary.getHandle()));
    necessary.wait();
  }
}

TEST(TriangleCount, SimpleDistributeVHosts) {
  int64_t num_hosts = pando::getPlaceDims().node.id;
  int64_t required_num_hosts = 3;
  if (num_hosts == required_num_hosts) {
    pando::Notification necessary;
    PANDO_CHECK(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore},
                                 &run_test_launch_assign_vhosts_to_host, necessary.getHandle()));
    necessary.wait();
  }
}

TEST(TriangleCount, SimpleBuildEdges2Send) {
  int64_t num_hosts = pando::getPlaceDims().node.id;
  int64_t required_num_hosts = 3;
  if (num_hosts == required_num_hosts) {
    pando::Notification necessary;
    PANDO_CHECK(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore},
                                 &run_test_launch_build_edges_to_send, necessary.getHandle()));
    necessary.wait();
  }
}

TEST(TriangleCount, SimpleEdgeExchange) {
  int64_t num_hosts = pando::getPlaceDims().node.id;
  int64_t required_num_hosts = 3;
  if (num_hosts == required_num_hosts) {
    pando::Notification necessary;
    PANDO_CHECK(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore},
                                 &run_test_launch_edge_exchange, necessary.getHandle()));
    necessary.wait();
  }
}
