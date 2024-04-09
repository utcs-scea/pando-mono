// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.
#ifndef PANDO_WF2_GALOIS_PARTIAL_PATTERN_H_
#define PANDO_WF2_GALOIS_PARTIAL_PATTERN_H_

#include "export.h"

#include "pando-rt/export.h"
#include "pando-rt/pando-rt.hpp"
#include "pando-wf2-galois/graph_ds.h"
#include <pando-lib-galois/graphs/dist_local_csr.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/sync/wait_group.hpp>
#include <pando-rt/memory/memory_guard.hpp>

namespace wf2 {
namespace partial {

using Vertex = wf::WMDVertex;
using Edge = wf::WMDEdge;
using WMDGraph = galois::DistLocalCSR<Vertex, Edge>;
using Graph = WMDGraph;

struct State {
  Graph graph;
  pando::Vector<bool> f2a_1;
  pando::Vector<bool> f2a_2;
  pando::Vector<bool> f2a;
  pando::Vector<bool> f2b_1;
  pando::Vector<bool> f2b_2;
  pando::Vector<bool> f2b_3;
  pando::Vector<bool> f2b;
  pando::Vector<bool> forum1;
  pando::Vector<bool> forum2;
  pando::Vector<bool> nyc;
  pando::Vector<std::int64_t> jihad;
  pando::Vector<bool> pub_ee;
  pando::Vector<bool> pub_nyc;
  pando::Vector<bool> pub;
  pando::Vector<bool> pub_seller;
  pando::Vector<bool> purchase_pc;
  pando::Vector<bool> purchase_bb;
  pando::Vector<bool> purchase_ee;
  pando::Vector<bool> purchase_ammo;
  pando::Vector<bool> ammo_dist;
  pando::Vector<std::uint64_t> ammo_buyer;
  pando::Vector<bool> ammo_seller;
  pando::Vector<std::time_t> trans_date;
  pando::Vector<std::time_t> forum_date;
  pando::Vector<bool> interesting_persons;
  pando::Vector<bool> matched_persons;
  pando::Vector<bool> sp12;

  explicit State(Graph g_) : graph(g_) {}
  State() = default;

  void initialize(std::uint64_t size) {
    PANDO_CHECK(f2a_1.initialize(size));
    PANDO_CHECK(f2a_2.initialize(size));
    PANDO_CHECK(f2a.initialize(size));
    PANDO_CHECK(f2b_1.initialize(size));
    PANDO_CHECK(f2b_2.initialize(size));
    PANDO_CHECK(f2b_3.initialize(size));
    PANDO_CHECK(f2b.initialize(size));
    PANDO_CHECK(forum1.initialize(size));
    PANDO_CHECK(forum2.initialize(size));
    PANDO_CHECK(nyc.initialize(size));
    PANDO_CHECK(jihad.initialize(size));
    PANDO_CHECK(pub_ee.initialize(size));
    PANDO_CHECK(pub_nyc.initialize(size));
    PANDO_CHECK(pub.initialize(size));
    PANDO_CHECK(pub_seller.initialize(size));
    PANDO_CHECK(purchase_pc.initialize(size));
    PANDO_CHECK(purchase_bb.initialize(size));
    PANDO_CHECK(purchase_ee.initialize(size));
    PANDO_CHECK(purchase_ammo.initialize(size));
    PANDO_CHECK(ammo_dist.initialize(size));
    PANDO_CHECK(ammo_buyer.initialize(size));
    PANDO_CHECK(ammo_seller.initialize(size));
    PANDO_CHECK(trans_date.initialize(size));
    PANDO_CHECK(forum_date.initialize(size));
    PANDO_CHECK(sp12.initialize(size));
    PANDO_CHECK(interesting_persons.initialize(size));
    PANDO_CHECK(matched_persons.initialize(size));
  }

  void deinitialize() {
    f2a_1.deinitialize();
    f2a_2.deinitialize();
    f2a.deinitialize();
    f2b_1.deinitialize();
    f2b_2.deinitialize();
    f2b_3.deinitialize();
    f2b.deinitialize();
    forum1.deinitialize();
    forum2.deinitialize();
    nyc.deinitialize();
    jihad.deinitialize();
    pub_ee.deinitialize();
    pub_nyc.deinitialize();
    pub.deinitialize();
    pub_seller.deinitialize();
    purchase_pc.deinitialize();
    purchase_bb.deinitialize();
    purchase_ee.deinitialize();
    purchase_ammo.deinitialize();
    ammo_dist.deinitialize();
    ammo_buyer.deinitialize();
    ammo_seller.deinitialize();
    trans_date.deinitialize();
    forum_date.deinitialize();
    sp12.deinitialize();
    matched_persons.deinitialize();
    interesting_persons.deinitialize();
  }
};

void partialMatch(pando::GlobalPtr<Graph>);
void matchForum1(State, Graph::EdgeHandle, Graph::VertexTopologyID, galois::WaitGroup::HandleType);
void matchForum2(State&, Graph::VertexTopologyID);
void matchFe2A2B(State, Graph::VertexTopologyID);
void matchPurchases(State, Graph::VertexTopologyID);
void matchAmmoPurchase(State&, Graph::VertexTopologyID&, Graph::EdgeHandle&);
void matchEEPurchase(State, Graph::VertexTopologyID, Graph::EdgeHandle);
void matchPersonPub(State, Graph::VertexTopologyID, Graph::EdgeHandle);
void matchPub(State, Graph::VertexTopologyID);
void matchPubEE(State&, Graph::VertexTopologyID&, Graph::EdgeHandle&);
void matchPubNyc(State&, Graph::VertexTopologyID&, Graph::EdgeHandle&);
void matchNyc(State&, Graph::VertexTopologyID&, Graph::EdgeHandle&);
void matchJihad(State&, Graph::VertexTopologyID&, Graph::EdgeHandle&);
void matchFe2A(State&, Graph::VertexTopologyID&, Graph::EdgeHandle&);
void matchFe2B(State&, Graph::VertexTopologyID&, Graph::EdgeHandle&);
void matchBasicPurchases(State&, Graph::VertexTopologyID&, Graph::EdgeHandle&);

} // namespace partial
} // namespace wf2

#endif // PANDO_WF2_GALOIS_PARTIAL_PATTERN_H_
