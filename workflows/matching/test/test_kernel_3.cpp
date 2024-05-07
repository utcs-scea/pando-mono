// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <gtest/gtest.h>

#include "pando-lib-galois/loops/do_all.hpp"
#include "pando-lib-galois/sync/wait_group.hpp"
#include "pando-rt/export.h"
#include "pando-rt/memory/memory_guard.hpp"
#include "pando-rt/pando-rt.hpp"
#include "pando-wf2-galois/graph_ds.h"
#include "pando-wf2-galois/import_wmd.hpp"
#include "pando-wf2-galois/partial_pattern.h"

std::string wmdFile() {
  return "/pando/graphs/wmd.csv";
}

std::uint64_t getVi(pando::GlobalPtr<wf2::partial::Graph> graph_ptr, std::uint64_t token) {
  wf2::partial::Graph graph = *graph_ptr;
  return graph.getVertexIndex(graph.getTopologyID(token));
}

wf2::partial::Graph::VertexTopologyID getTopID(pando::GlobalPtr<wf2::partial::Graph> graph_ptr,
                                               std::uint64_t token) {
  wf2::partial::Graph graph = *graph_ptr;
  return graph.getTopologyID(token);
}

TEST(Graph, GRAPH_INIT) {
  pando::GlobalPtr<wf2::partial::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::partial::Graph graph = *graph_ptr;
  EXPECT_EQ(graph.size(), 25);
}

TEST(WF2_Partial, BASIC_PURCHASES) {
  pando::GlobalPtr<wf2::partial::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::partial::Graph graph = *(graph_ptr);
  wf2::partial::State state(graph);
  state.initialize(graph.size());

  for (wf2::partial::Graph::VertexTopologyID lid : graph.vertices()) {
    for (wf2::partial::Graph::EdgeHandle eh : state.graph.edges(lid)) {
      wf2::partial::Edge edge = state.graph.getEdgeData(eh);
      wf2::partial::Graph::VertexTopologyID dst = state.graph.getEdgeDst(eh);
      wf2::partial::Vertex node = state.graph.getData(lid);
      wf2::partial::Vertex dst_node = state.graph.getData(dst);
      if (node.type == agile::TYPES::PERSON && edge.type == agile::TYPES::PURCHASE &&
          dst_node.type == agile::TYPES::PERSON) {
        matchBasicPurchases(state, lid, eh);
      }
    }
  }
  EXPECT_EQ(state.purchase_pc[getVi(graph_ptr, 1)], true);
  EXPECT_EQ(state.purchase_bb[getVi(graph_ptr, 1)], true);
}

TEST(WF2_Partial, FORUM_EVENT_2B) {
  pando::GlobalPtr<wf2::partial::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::partial::Graph graph = *(graph_ptr);
  wf2::partial::State state(graph);
  state.initialize(graph.size());

  for (wf2::partial::Graph::VertexTopologyID lid : graph.vertices()) {
    for (wf2::partial::Graph::EdgeHandle eh : state.graph.edges(lid)) {
      wf2::partial::Edge edge = state.graph.getEdgeData(eh);
      wf2::partial::Graph::VertexTopologyID dst = state.graph.getEdgeDst(eh);
      wf2::partial::Vertex node = state.graph.getData(lid);
      wf2::partial::Vertex dst_node = state.graph.getData(dst);
      if (node.type == agile::TYPES::FORUMEVENT && edge.type == agile::TYPES::HASTOPIC &&
          dst_node.type == agile::TYPES::TOPIC) {
        matchFe2B(state, lid, eh);
      }
    }
  }
  EXPECT_EQ(state.f2b_1[getVi(graph_ptr, 1101)], true);
  EXPECT_EQ(state.f2b_2[getVi(graph_ptr, 1101)], true);
  EXPECT_EQ(state.f2b_3[getVi(graph_ptr, 1101)], true);
}

TEST(WF2_Partial, FORUM_EVENT_2A) {
  pando::GlobalPtr<wf2::partial::Graph> graph_ptr = wf::ImportWMDGraph(wmdFile());
  wf2::partial::Graph graph = *(graph_ptr);
  wf2::partial::State state(graph);
  state.initialize(graph.size());

  for (wf2::partial::Graph::VertexTopologyID lid : graph.vertices()) {
    for (wf2::partial::Graph::EdgeHandle eh : state.graph.edges(lid)) {
      wf2::partial::Edge edge = state.graph.getEdgeData(eh);
      wf2::partial::Graph::VertexTopologyID dst = state.graph.getEdgeDst(eh);
      wf2::partial::Vertex node = state.graph.getData(lid);
      wf2::partial::Vertex dst_node = state.graph.getData(dst);
      if (node.type == agile::TYPES::FORUMEVENT && edge.type == agile::TYPES::HASTOPIC &&
          dst_node.type == agile::TYPES::TOPIC) {
        matchFe2A(state, lid, eh);
      }
    }
  }
  EXPECT_EQ(state.f2a_1[getVi(graph_ptr, 1101)], true);
  EXPECT_EQ(state.f2a_2[getVi(graph_ptr, 1101)], true);
}
