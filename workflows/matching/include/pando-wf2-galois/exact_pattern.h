// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.
#ifndef PANDO_WF2_GALOIS_EXACT_PATTERN_H_
#define PANDO_WF2_GALOIS_EXACT_PATTERN_H_

#include "export.h"

#include "graph_ds.h"
#include "pando-rt/export.h"
#include "pando-rt/pando-rt.hpp"
#include <pando-lib-galois/graphs/dist_local_csr.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/sync/wait_group.hpp>
#include <pando-lib-galois/utility/agile_schema.hpp>
#include <pando-rt/memory/memory_guard.hpp>

namespace wf2 {
namespace exact {

using Vertex = wf::WMDVertex;
using Edge = wf::WMDEdge;
using WMDGraph = galois::DistLocalCSR<Vertex, Edge>;
using Graph = WMDGraph;

struct State {
  Graph g;
  galois::WaitGroup::HandleType wgh;

  State(Graph g_, galois::WaitGroup::HandleType wgh_) : g(g_), wgh(wgh_) {}
  explicit State(Graph g_) : g(g_) {}
  State() = default;
};

struct EEState : public State {
  pando::Vector<bool> ee_vec;
  pando::Vector<bool> ee_seller_vec;
  pando::Vector<bool> ee_pub_vec;
  pando::Vector<bool> ee_topic_vec;
  pando::Vector<bool> ee_org_vec;

  EEState(Graph g_, galois::WaitGroup::HandleType wgh_) : State(g_, wgh_) {}
  explicit EEState(Graph g_) : State(g_) {}
  EEState() = default;

  void initialize(std::uint64_t size) {
    PANDO_CHECK(ee_vec.initialize(size));
    PANDO_CHECK(ee_seller_vec.initialize(size));
    PANDO_CHECK(ee_pub_vec.initialize(size));
    PANDO_CHECK(ee_topic_vec.initialize(size));
    PANDO_CHECK(ee_org_vec.initialize(size));
  }

  void deinitialize() {
    ee_vec.deinitialize();
    ee_seller_vec.deinitialize();
    ee_pub_vec.deinitialize();
    ee_topic_vec.deinitialize();
    ee_org_vec.deinitialize();
  }
};

struct ForumEvent2BState : public State {
  pando::Vector<bool> williamsburg;
  pando::Vector<bool> explosion;
  pando::Vector<bool> bomb;

  ForumEvent2BState(Graph g_, galois::WaitGroup::HandleType wgh_) : State(g_, wgh_) {}
  explicit ForumEvent2BState(Graph g_) : State(g_) {}
  ForumEvent2BState() = default;

  void initialize(std::uint64_t size) {
    PANDO_CHECK(williamsburg.initialize(size));
    PANDO_CHECK(explosion.initialize(size));
    PANDO_CHECK(bomb.initialize(size));
  }

  void deinitialize() {
    williamsburg.deinitialize();
    explosion.deinitialize();
    bomb.deinitialize();
  }
};

struct ForumEvent2AState : public State {
  pando::Vector<bool> outdoors;
  pando::Vector<bool> prospect_park;

  ForumEvent2AState(Graph g_, galois::WaitGroup::HandleType wgh_) : State(g_, wgh_) {}
  explicit ForumEvent2AState(Graph g_) : State(g_) {}
  ForumEvent2AState() = default;

  void initialize(std::uint64_t size) {
    PANDO_CHECK(outdoors.initialize(size));
    PANDO_CHECK(prospect_park.initialize(size));
  }

  void deinitialize() {
    outdoors.deinitialize();
    prospect_park.deinitialize();
  }
};

struct ForumState : public State {
  pando::Vector<bool> two_a;
  pando::Vector<bool> two_b;
  pando::Vector<bool> jihad;
  pando::Vector<bool> nyc;
  pando::Vector<bool> forum1;
  pando::Vector<bool> forum2;
  pando::Vector<std::time_t> forum_min_time;
  ForumEvent2AState fe_2a_state;
  ForumEvent2BState fe_2b_state;

  ForumState(Graph g_, galois::WaitGroup::HandleType wgh_)
      : State(g_, wgh_), fe_2a_state(g_, wgh_), fe_2b_state(g_, wgh_) {}
  explicit ForumState(Graph g_) : State(g_), fe_2a_state(g_), fe_2b_state(g_) {}
  ForumState() = default;

  void initialize(std::uint64_t size) {
    PANDO_CHECK(two_a.initialize(size));
    PANDO_CHECK(two_b.initialize(size));
    PANDO_CHECK(jihad.initialize(size));
    PANDO_CHECK(nyc.initialize(size));
    PANDO_CHECK(forum_min_time.initialize(size));
    PANDO_CHECK(forum1.initialize(size));
    PANDO_CHECK(forum2.initialize(size));
    fe_2a_state.initialize(size);
    fe_2b_state.initialize(size);
  }

  void deinitialize() {
    two_a.deinitialize();
    two_b.deinitialize();
    jihad.deinitialize();
    nyc.deinitialize();
    forum_min_time.deinitialize();
    forum1.deinitialize();
    forum2.deinitialize();
    fe_2a_state.deinitialize();
    fe_2b_state.deinitialize();
  }
};

struct PurchaseState : public State {
  pando::Vector<bool> prchsd_vec;
  pando::Vector<bool> ammo_vec;
  EEState ee_state;
  ForumState forum_state;

  PurchaseState(Graph g_, galois::WaitGroup::HandleType wgh_)
      : State(g_, wgh_), ee_state(g_, wgh_), forum_state(g_, wgh_) {}
  explicit PurchaseState(Graph g_) : State(g_), ee_state(g_), forum_state(g_) {}
  PurchaseState() = default;

  void initialize(std::uint64_t size) {
    PANDO_CHECK(prchsd_vec.initialize(size));
    PANDO_CHECK(ammo_vec.initialize(size));
    ee_state.initialize(size);
    forum_state.initialize(size);
  }

  void deinitialize() {
    prchsd_vec.deinitialize();
    ammo_vec.deinitialize();
    ee_state.deinitialize();
    forum_state.deinitialize();
  }
};

void patternMatch(pando::GlobalPtr<Graph>);
void purchaseMatch(PurchaseState&, Graph::VertexTopologyID);
void eeTopicMatch(EEState&, Graph::EdgeHandle);
void eeOrgMatch(EEState&, Graph::EdgeHandle);
void eePublicationMatch(EEState&, Graph::EdgeHandle);
void eeSellerMatch(EEState&, Graph::EdgeHandle);
void eeMatch(EEState&, Graph::EdgeHandle);
void forumDateMatch(ForumState&, Graph::VertexTopologyID);
void ammoMatch(PurchaseState&, Graph::EdgeHandle);
void forum1(ForumState&, Graph::VertexTopologyID);
void forum2(ForumState&, Graph::VertexTopologyID, std::time_t);
bool forumSubPattern(ForumState&, Graph::VertexTopologyID, std::time_t);
void forumNYCMatch(ForumState&, Graph::EdgeHandle);
void forumNYCTopicMatch(ForumState&, Graph::EdgeHandle);
void forumFE2AMatch(ForumState&, Graph::EdgeHandle);
void forumFE2ATopicMatch(ForumEvent2AState&, Graph::EdgeHandle);
void forumFE2BMatch(ForumState&, Graph::EdgeHandle);
void forumFE2BTopicMatch(ForumEvent2BState&, Graph::EdgeHandle);
void forumFEJihadMatch(ForumState&, Graph::EdgeHandle);
void forumFEJihadTopicMatch(ForumState&, Graph::EdgeHandle);

} // namespace exact
} // namespace wf2

#endif // PANDO_WF2_GALOIS_EXACT_PATTERN_H_
