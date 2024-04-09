// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>
#include <sstream>
#include <string>

#include "pando-lib-galois/loops/do_all.hpp"
#include "pando-lib-galois/sync/wait_group.hpp"
#include "pando-rt/export.h"
#include "pando-rt/memory/memory_guard.hpp"
#include "pando-rt/pando-rt.hpp"
#include "pando-wf2-galois/exact_pattern.h"
#include "pando-wf2-galois/graph_ds.h"
#include "pando-wf2-galois/import_wmd.hpp"

std::string wmdFile() {
  return "/pando/graphs/wmd.csv";
}

std::uint64_t getVi(pando::GlobalPtr<wf2::exact::Graph> graph_ptr, std::uint64_t token) {
  wf2::exact::Graph graph = *graph_ptr;
  return graph.getVertexIndex(graph.getTopologyID(token));
}

wf2::exact::Graph::VertexTopologyID getTopID(pando::GlobalPtr<wf2::exact::Graph> graph_ptr,
                                             std::uint64_t token) {
  wf2::exact::Graph graph = *graph_ptr;
  return graph.getTopologyID(token);
}

TEST(Init, CP) {
  auto place = pando::getCurrentPlace();
  EXPECT_EQ(place.core, (pando::CoreIndex{-1, -1}));
}

TEST(Graph, GRAPH_INIT) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  EXPECT_EQ(graph.size(), 25);
}

TEST(WF2_Exact, PURCHASE) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  uint64_t num_nodes = graph.size();

  wf2::exact::PurchaseState purchase_state(graph);
  purchase_state.initialize(num_nodes);

  galois::doAll(purchase_state, graph.vertices(), &wf2::exact::purchaseMatch);

  EXPECT_EQ(static_cast<bool>(purchase_state.ammo_vec[getVi(graph_ptr, 4)]), true);
  EXPECT_EQ(static_cast<bool>(purchase_state.ammo_vec[getVi(graph_ptr, 5)]), false);
}

TEST(WF2_Exact, PURCHASE_AMMO) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  uint64_t num_nodes = graph.size();

  wf2::exact::PurchaseState purchase_state(graph);
  purchase_state.initialize(num_nodes);

  galois::doAll(purchase_state, graph.edges(getTopID(graph_ptr, 1)), &wf2::exact::ammoMatch);

  EXPECT_EQ(static_cast<bool>(purchase_state.ammo_vec[getVi(graph_ptr, 4)]), true);
  EXPECT_EQ(static_cast<bool>(purchase_state.ammo_vec[getVi(graph_ptr, 5)]), false);
}

TEST(WF2_Exact, EE_SP_TOPIC) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  std::uint64_t num_nodes = graph.size();

  wf2::exact::EEState ee_state(graph);
  ee_state.initialize(num_nodes);

  galois::doAll(ee_state, graph.edges(getTopID(graph_ptr, 101)), &wf2::exact::eeTopicMatch);

  EXPECT_EQ(static_cast<bool>(ee_state.ee_topic_vec[getVi(graph_ptr, 43035)]), true);
  EXPECT_EQ(static_cast<bool>(ee_state.ee_topic_vec[getVi(graph_ptr, 106)]), false);
}

TEST(WF2_Exact, EE_SP_ORG) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  std::uint64_t num_nodes = graph.size();

  wf2::exact::EEState ee_state(graph);
  ee_state.initialize(num_nodes);

  galois::doAll(ee_state, graph.edges(getTopID(graph_ptr, 101)), &wf2::exact::eeOrgMatch);

  EXPECT_EQ(static_cast<bool>(ee_state.ee_org_vec[getVi(graph_ptr, 103)]), true);
  EXPECT_EQ(static_cast<bool>(ee_state.ee_org_vec[getVi(graph_ptr, 104)]), false);
}

TEST(WF2_Exact, EE_SP_PUB) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  std::uint64_t num_nodes = graph.size();

  wf2::exact::EEState ee_state(graph);
  ee_state.initialize(num_nodes);

  galois::doAll(ee_state, graph.edges(getTopID(graph_ptr, 5)), &wf2::exact::eePublicationMatch);

  EXPECT_EQ(static_cast<bool>(ee_state.ee_pub_vec[getVi(graph_ptr, 101)]), true);
  EXPECT_EQ(static_cast<bool>(ee_state.ee_pub_vec[getVi(graph_ptr, 102)]), false);
}

TEST(WF2_Exact, EE_SP_SELLER) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  std::uint64_t num_nodes = graph.size();

  wf2::exact::EEState ee_state(graph);
  ee_state.initialize(num_nodes);

  galois::doAll(ee_state, graph.edges(getTopID(graph_ptr, 1)), &wf2::exact::eeSellerMatch);

  EXPECT_EQ(static_cast<bool>(ee_state.ee_seller_vec[getVi(graph_ptr, 5)]), true);
  EXPECT_EQ(static_cast<bool>(ee_state.ee_seller_vec[getVi(graph_ptr, 6)]), false);
}

TEST(WF2_Exact, FORUM_2A_TOPIC) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  std::uint64_t num_nodes = graph.size();

  wf2::exact::ForumState forum_state(graph);
  forum_state.initialize(num_nodes);

  galois::doAll(forum_state.fe_2a_state, graph.edges(getTopID(graph_ptr, 1101)),
                &wf2::exact::forumFE2ATopicMatch);

  EXPECT_EQ(static_cast<bool>(forum_state.fe_2a_state.outdoors[getVi(graph_ptr, 69871376)]), true);
  EXPECT_EQ(static_cast<bool>(forum_state.fe_2a_state.prospect_park[getVi(graph_ptr, 69871376)]),
            false);
  EXPECT_EQ(static_cast<bool>(forum_state.fe_2a_state.outdoors[getVi(graph_ptr, 1049632)]), false);
  EXPECT_EQ(static_cast<bool>(forum_state.fe_2a_state.prospect_park[getVi(graph_ptr, 1049632)]),
            true);
  EXPECT_EQ(static_cast<bool>(forum_state.fe_2a_state.outdoors[getVi(graph_ptr, 100)]), false);
  EXPECT_EQ(static_cast<bool>(forum_state.fe_2a_state.outdoors[getVi(graph_ptr, 100)]), false);
}

TEST(WF2_Exact, FORUM_2A) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  std::uint64_t num_nodes = graph.size();

  wf2::exact::ForumState forum_state(graph);
  forum_state.initialize(num_nodes);

  galois::doAll(forum_state, graph.edges(getTopID(graph_ptr, 1102)), &wf2::exact::forumFE2AMatch);

  EXPECT_EQ(static_cast<bool>(forum_state.two_a[getVi(graph_ptr, 1101)]), true);
  EXPECT_EQ(static_cast<bool>(forum_state.two_a[getVi(graph_ptr, 1103)]), false);
  EXPECT_EQ(static_cast<bool>(forum_state.two_a[getVi(graph_ptr, 1104)]), false);
}

TEST(WF2_Exact, FORUM_2B_TOPIC) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  std::uint64_t num_nodes = graph.size();

  wf2::exact::ForumState forum_state(graph);
  forum_state.initialize(num_nodes);

  galois::doAll(forum_state.fe_2b_state, graph.edges(getTopID(graph_ptr, 1101)),
                &wf2::exact::forumFE2BTopicMatch);

  EXPECT_EQ(static_cast<bool>(forum_state.fe_2b_state.williamsburg[getVi(graph_ptr, 771572)]),
            true);
  EXPECT_EQ(static_cast<bool>(forum_state.fe_2b_state.explosion[getVi(graph_ptr, 771572)]), false);
  EXPECT_EQ(static_cast<bool>(forum_state.fe_2b_state.bomb[getVi(graph_ptr, 771572)]), false);
  EXPECT_EQ(static_cast<bool>(forum_state.fe_2b_state.explosion[getVi(graph_ptr, 179057)]), true);
  EXPECT_EQ(static_cast<bool>(forum_state.fe_2b_state.williamsburg[getVi(graph_ptr, 179057)]),
            false);
  EXPECT_EQ(static_cast<bool>(forum_state.fe_2b_state.bomb[getVi(graph_ptr, 179057)]), false);
  EXPECT_EQ(static_cast<bool>(forum_state.fe_2b_state.bomb[getVi(graph_ptr, 127197)]), true);
  EXPECT_EQ(static_cast<bool>(forum_state.fe_2b_state.williamsburg[getVi(graph_ptr, 127197)]),
            false);
  EXPECT_EQ(static_cast<bool>(forum_state.fe_2b_state.explosion[getVi(graph_ptr, 127197)]), false);
}

TEST(WF2_Exact, FORUM_2B) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  std::uint64_t num_nodes = graph.size();

  wf2::exact::ForumState forum_state(graph);
  forum_state.initialize(num_nodes);

  galois::doAll(forum_state, graph.edges(getTopID(graph_ptr, 1102)), &wf2::exact::forumFE2BMatch);

  EXPECT_EQ(static_cast<bool>(forum_state.two_b[getVi(graph_ptr, 1101)]), true);
  EXPECT_EQ(static_cast<bool>(forum_state.two_b[getVi(graph_ptr, 1103)]), false);
  EXPECT_EQ(static_cast<bool>(forum_state.two_b[getVi(graph_ptr, 1104)]), false);
}

TEST(WF2_Exact, FORUM_JIHAD_TOPIC) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  std::uint64_t num_nodes = graph.size();

  wf2::exact::ForumState forum_state(graph);
  forum_state.initialize(num_nodes);

  galois::doAll(forum_state, graph.edges(getTopID(graph_ptr, 1101)),
                &wf2::exact::forumFEJihadTopicMatch);

  EXPECT_EQ(static_cast<bool>(forum_state.jihad[getVi(graph_ptr, 44311)]), true);
  EXPECT_EQ(static_cast<bool>(forum_state.jihad[getVi(graph_ptr, 100)]), false);
}

TEST(WF2_Exact, FORUM_NYC_TOPIC) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  std::uint64_t num_nodes = graph.size();

  wf2::exact::ForumState forum_state(graph);
  forum_state.initialize(num_nodes);

  galois::doAll(forum_state, graph.edges(getTopID(graph_ptr, 1102)),
                &wf2::exact::forumNYCTopicMatch);

  EXPECT_EQ(static_cast<bool>(forum_state.nyc[getVi(graph_ptr, 60)]), true);
  EXPECT_EQ(static_cast<bool>(forum_state.nyc[getVi(graph_ptr, 100)]), false);
}

TEST(WF2_Exact, FORUM_JIHAD) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  std::uint64_t num_nodes = graph.size();

  wf2::exact::ForumState forum_state(graph);
  forum_state.initialize(num_nodes);

  galois::doAll(forum_state, graph.edges(getTopID(graph_ptr, 1102)),
                &wf2::exact::forumFEJihadMatch);

  EXPECT_EQ(static_cast<bool>(forum_state.jihad[getVi(graph_ptr, 1101)]), true);
  EXPECT_EQ(static_cast<bool>(forum_state.jihad[getVi(graph_ptr, 1103)]), true);
  EXPECT_EQ(static_cast<bool>(forum_state.jihad[getVi(graph_ptr, 1104)]), false);
}

TEST(WF2_Exact, FORUM_NYC) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  std::uint64_t num_nodes = graph.size();

  wf2::exact::ForumState forum_state(graph);
  forum_state.initialize(num_nodes);

  galois::doAll(forum_state, graph.edges(getTopID(graph_ptr, 1101)), &wf2::exact::forumNYCMatch);

  EXPECT_EQ(static_cast<bool>(forum_state.nyc[getVi(graph_ptr, 1102)]), true);
}

TEST(WF2_Exact, FORUM_DATE) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  std::uint64_t num_nodes = graph.size();

  wf2::exact::ForumState forum_state(graph);
  forum_state.initialize(num_nodes);

  galois::doAll(forum_state, graph.vertices(), &wf2::exact::forumDateMatch);

  EXPECT_EQ(static_cast<std::time_t>(forum_state.forum_min_time[getVi(graph_ptr, 1102)]),
            1483747200);
}

TEST(WF2_Exact, FORUM_1) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  std::uint64_t num_nodes = graph.size();

  wf2::exact::PurchaseState forum_state(graph);
  forum_state.initialize(num_nodes);

  galois::doAll(forum_state.forum_state, graph.vertices(), &wf2::exact::forum1);

  EXPECT_EQ(static_cast<bool>(forum_state.forum_state.forum1[getVi(graph_ptr, 1)]), true);
}

TEST(WF2_Exact, FORUM_2) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  std::uint64_t num_nodes = graph.size();

  wf2::exact::ForumState forum_state(graph);
  forum_state.initialize(num_nodes);

  std::time_t t = 1701053242;
  galois::doAll(forum_state, graph.vertices(), &wf2::exact::forumDateMatch);
  wf2::exact::forum2(forum_state, getTopID(graph_ptr, 1), t);

  EXPECT_EQ(static_cast<bool>(forum_state.forum2[getVi(graph_ptr, 1)]), true);
}

TEST(WF2_Exact, FORUM_SP) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  std::uint64_t num_nodes = graph.size();

  wf2::exact::PurchaseState purchase_state(graph);
  purchase_state.initialize(num_nodes);

  std::time_t t = 1701053242;
  galois::doAll(purchase_state.forum_state, graph.vertices(), &wf2::exact::forumDateMatch);
  wf2::exact::forumSubPattern(purchase_state.forum_state, getTopID(graph_ptr, 1), t);
}

TEST(WF2_Exact, WMD) {
  pando::GlobalPtr<wf2::exact::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::exact::Graph graph = *graph_ptr;
  std::uint64_t num_nodes = graph.size();

  wf2::exact::PurchaseState purchase_state(graph);
  purchase_state.initialize(num_nodes);

  wf2::exact::purchaseMatch(purchase_state, getTopID(graph_ptr, 1));
}
