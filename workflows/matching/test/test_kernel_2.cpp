// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <sstream>
#include <string>

#include "pando-lib-galois/loops/do_all.hpp"
#include "pando-lib-galois/sync/wait_group.hpp"
#include "pando-rt/export.h"
#include "pando-rt/memory/memory_guard.hpp"
#include "pando-rt/pando-rt.hpp"
#include "pando-wf2-galois/approx_match.h"
#include "pando-wf2-galois/graph_ds.h"
#include "pando-wf2-galois/import_wmd.hpp"

std::string wmdFile() {
  return "/pando/graphs/wmd.csv";
}
std::string patternFile() {
  return "/pando/graphs/pattern.csv";
}

std::uint64_t getVi(pando::GlobalPtr<wf2::approx::Graph> graph_ptr, std::uint64_t token) {
  wf2::approx::Graph graph = *graph_ptr;
  return graph.getVertexIndex(graph.getTopologyID(token));
}

wf2::approx::Graph::VertexTopologyID getTopID(pando::GlobalPtr<wf2::approx::Graph> graph_ptr,
                                              std::uint64_t token) {
  wf2::approx::Graph graph = *graph_ptr;
  return graph.getTopologyID(token);
}

TEST(Graph, GRAPH_INIT) {
  pando::GlobalPtr<wf2::approx::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::approx::Graph graph = *graph_ptr;
  EXPECT_EQ(graph.size(), 25);
}

TEST(WF2_Approx, TRIPLES_CHECK) {
  pando::GlobalPtr<wf2::approx::Graph> lhs_ptr = wf::ImportWMDGraph(patternFile());
  pando::GlobalPtr<wf2::approx::Graph> rhs_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::approx::Graph lhs = *(lhs_ptr);
  wf2::approx::Graph rhs = *(rhs_ptr);
  wf2::approx::State state(lhs, rhs);
  state.initialize();

  galois::doAll(state.state_lhs, lhs.vertices(), &wf2::approx::matchTriples);
  int i = getVi(lhs_ptr, 1);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_sale_person_bomb_bath[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_sale_person_pressure_cooker[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_sale_person_ammunition[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_sale_person_electronics[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_purchase_person_bomb_bath[i]), 1);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_purchase_person_pressure_cooker[i]), 1);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_purchase_person_ammunition[i]), 1);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_purchase_person_electronics[i]), 1);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_author_forumevent[i]), 3);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_author_publication[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.forum_includes_forumevent[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.forum_hastopic_topic_nyc[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.forumevent_hastopic_topic_bomb[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.forumevent_hastopic_topic_explosion[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.forumevent_hastopic_topic_williamsburg[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.forumevent_hastopic_topic_outdoors[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.forumevent_hastopic_topic_prospect_park[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.forumevent_hastopic_topic_jihad[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.publication_hasorg_topic_near_nyc[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.publication_hastopic_topic_electrical_eng[i]), 0);

  i = getVi(lhs_ptr, 2);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_sale_person_bomb_bath[i]), 1);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_sale_person_pressure_cooker[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_sale_person_ammunition[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_sale_person_electronics[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_purchase_person_bomb_bath[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_purchase_person_pressure_cooker[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_purchase_person_ammunition[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_purchase_person_electronics[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_author_forumevent[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.person_author_publication[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.forum_includes_forumevent[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.forum_hastopic_topic_nyc[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.forumevent_hastopic_topic_bomb[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.forumevent_hastopic_topic_explosion[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.forumevent_hastopic_topic_williamsburg[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.forumevent_hastopic_topic_outdoors[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.forumevent_hastopic_topic_prospect_park[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.forumevent_hastopic_topic_jihad[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.publication_hasorg_topic_near_nyc[i]), 0);
  EXPECT_EQ(static_cast<uint64_t>(state.state_lhs.publication_hastopic_topic_electrical_eng[i]), 0);
}

TEST(WF2_Approx, TRIPLES_SIMILARITY) {
  pando::GlobalPtr<wf2::approx::Graph> lhs_ptr = wf::ImportWMDGraph(patternFile());
  pando::GlobalPtr<wf2::approx::Graph> rhs_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::approx::Graph lhs = *(lhs_ptr);
  wf2::approx::Graph rhs = *(rhs_ptr);
  wf2::approx::State state(lhs, rhs);
  state.initialize();

  galois::doAll(state.state_lhs, lhs.vertices(), &wf2::approx::matchTriples);
  galois::doAll(state.state_rhs, rhs.vertices(), &wf2::approx::matchTriples);
  wf2::approx::calculateSimilarity(state);

  pando::Vector<wf2::approx::node_sim> lhs_list = state.state_lhs.similarity[getVi(lhs_ptr, 1)];
  EXPECT_NEAR(static_cast<wf2::approx::node_sim>(lhs_list[getVi(rhs_ptr, 1)]).similarity, 2.7735,
              0.0001);
  lhs_list = state.state_lhs.similarity[getVi(lhs_ptr, 4)];
  EXPECT_NEAR(static_cast<wf2::approx::node_sim>(lhs_list[getVi(rhs_ptr, 4)]).similarity, 2,
              0.0001);
  lhs_list = state.state_lhs.similarity[getVi(lhs_ptr, 10)];
  EXPECT_NEAR(static_cast<wf2::approx::node_sim>(lhs_list[getVi(rhs_ptr, 1105)]).similarity,
              1.73205, 0.0001);
  lhs_list = state.state_lhs.similarity[getVi(lhs_ptr, 12)];
  EXPECT_NEAR(static_cast<wf2::approx::node_sim>(lhs_list[getVi(rhs_ptr, 1102)]).similarity,
              2.91043, 0.0001);
}
