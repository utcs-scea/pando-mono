// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.
#ifndef PANDO_WF2_GALOIS_APPROX_MATCH_H_
#define PANDO_WF2_GALOIS_APPROX_MATCH_H_

#include "export.h"

#include "pando-rt/export.h"
#include "pando-rt/pando-rt.hpp"
#include "pando-wf2-galois/graph_ds.h"
#include <pando-lib-galois/graphs/dist_local_csr.hpp>

namespace wf2 {
namespace approx {

using Vertex = wf::WMDVertex;
using Edge = wf::WMDEdge;
using Graph = galois::DistLocalCSR<Vertex, Edge>;

struct node_sim {
  std::uint64_t lid;
  double similarity;
  std::uint64_t token;
  node_sim(std::uint64_t lid_, double similarity_, std::uint64_t token_)
      : lid(lid_), similarity(similarity_), token(token_) {}
  node_sim() = default;
};

struct GraphState {
  Graph graph;
  pando::Vector<std::uint64_t> person_sale_person_bomb_bath;
  pando::Vector<std::uint64_t> person_sale_person_pressure_cooker;
  pando::Vector<std::uint64_t> person_sale_person_ammunition;
  pando::Vector<std::uint64_t> person_sale_person_electronics;
  pando::Vector<std::uint64_t> person_purchase_person_bomb_bath;
  pando::Vector<std::uint64_t> person_purchase_person_pressure_cooker;
  pando::Vector<std::uint64_t> person_purchase_person_ammunition;
  pando::Vector<std::uint64_t> person_purchase_person_electronics;
  pando::Vector<std::uint64_t> person_author_forumevent;
  pando::Vector<std::uint64_t> person_author_publication;
  pando::Vector<std::uint64_t> forum_includes_forumevent;
  pando::Vector<std::uint64_t> forum_hastopic_topic_nyc;
  pando::Vector<std::uint64_t> forumevent_hastopic_topic_bomb;
  pando::Vector<std::uint64_t> forumevent_hastopic_topic_explosion;
  pando::Vector<std::uint64_t> forumevent_hastopic_topic_williamsburg;
  pando::Vector<std::uint64_t> forumevent_hastopic_topic_outdoors;
  pando::Vector<std::uint64_t> forumevent_hastopic_topic_prospect_park;
  pando::Vector<std::uint64_t> forumevent_hastopic_topic_jihad;
  pando::Vector<std::uint64_t> publication_hasorg_topic_near_nyc;
  pando::Vector<std::uint64_t> publication_hastopic_topic_electrical_eng;

  pando::Vector<pando::Vector<node_sim>> similarity;
  pando::Vector<bool> new_matched;
  pando::Vector<bool> matched;
  pando::Vector<node_sim> match;

  explicit GraphState(Graph g_) : graph(g_) {}
  GraphState() = default;

  void initialize(std::uint64_t other_size) {
    std::uint64_t size = graph.size();
    PANDO_CHECK(person_sale_person_bomb_bath.initialize(size));
    PANDO_CHECK(person_sale_person_pressure_cooker.initialize(size));
    PANDO_CHECK(person_sale_person_ammunition.initialize(size));
    PANDO_CHECK(person_sale_person_electronics.initialize(size));
    PANDO_CHECK(person_purchase_person_bomb_bath.initialize(size));
    PANDO_CHECK(person_purchase_person_pressure_cooker.initialize(size));
    PANDO_CHECK(person_purchase_person_ammunition.initialize(size));
    PANDO_CHECK(person_purchase_person_electronics.initialize(size));
    PANDO_CHECK(person_author_forumevent.initialize(size));
    PANDO_CHECK(person_author_publication.initialize(size));
    PANDO_CHECK(forum_includes_forumevent.initialize(size));
    PANDO_CHECK(forum_hastopic_topic_nyc.initialize(size));
    PANDO_CHECK(forumevent_hastopic_topic_bomb.initialize(size));
    PANDO_CHECK(forumevent_hastopic_topic_explosion.initialize(size));
    PANDO_CHECK(forumevent_hastopic_topic_williamsburg.initialize(size));
    PANDO_CHECK(forumevent_hastopic_topic_outdoors.initialize(size));
    PANDO_CHECK(forumevent_hastopic_topic_prospect_park.initialize(size));
    PANDO_CHECK(forumevent_hastopic_topic_jihad.initialize(size));
    PANDO_CHECK(publication_hasorg_topic_near_nyc.initialize(size));
    PANDO_CHECK(publication_hastopic_topic_electrical_eng.initialize(size));

    PANDO_CHECK(similarity.initialize(size));
    for (pando::GlobalRef<pando::Vector<node_sim>> s : similarity) {
      pando::Vector<node_sim> empty_s;
      PANDO_CHECK(empty_s.initialize(other_size));
      s = empty_s;
    }
    PANDO_CHECK(new_matched.initialize(size));
    PANDO_CHECK(matched.initialize(size));
    PANDO_CHECK(match.initialize(size));
  }

  void deinitialize() {
    person_sale_person_bomb_bath.deinitialize();
    person_sale_person_pressure_cooker.deinitialize();
    person_sale_person_ammunition.deinitialize();
    person_sale_person_electronics.deinitialize();
    person_purchase_person_bomb_bath.deinitialize();
    person_purchase_person_pressure_cooker.deinitialize();
    person_purchase_person_ammunition.deinitialize();
    person_purchase_person_electronics.deinitialize();
    person_author_forumevent.deinitialize();
    person_author_publication.deinitialize();
    forum_includes_forumevent.deinitialize();
    forum_hastopic_topic_nyc.deinitialize();
    forumevent_hastopic_topic_bomb.deinitialize();
    forumevent_hastopic_topic_explosion.deinitialize();
    forumevent_hastopic_topic_williamsburg.deinitialize();
    forumevent_hastopic_topic_outdoors.deinitialize();
    forumevent_hastopic_topic_prospect_park.deinitialize();
    forumevent_hastopic_topic_jihad.deinitialize();
    publication_hasorg_topic_near_nyc.deinitialize();
    publication_hastopic_topic_electrical_eng.deinitialize();
    for (pando::Vector<node_sim> s : similarity) {
      s.deinitialize();
    }
    similarity.deinitialize();
    match.deinitialize();
    matched.deinitialize();
    new_matched.deinitialize();
  }
};

struct State {
  GraphState state_lhs;
  GraphState state_rhs;
  pando::GlobalPtr<std::uint64_t> match_count_ptr;

public:
  State() = default;
  State(Graph g_lhs, Graph g_rhs) : state_lhs(g_lhs), state_rhs(g_rhs) {}

  void initialize() {
    state_lhs.initialize(state_rhs.graph.size());
    state_rhs.initialize(state_lhs.graph.size());
    match_count_ptr = static_cast<decltype(match_count_ptr)>(
        pando::getDefaultMainMemoryResource()->allocate(sizeof(std::uint64_t)));
  }

  void deinitialize() {
    state_lhs.deinitialize();
    state_rhs.deinitialize();
  }
};

void match(pando::GlobalPtr<Graph>, pando::GlobalPtr<Graph>, int k);

void matchTriples(GraphState, Graph::VertexTopologyID);
void matchTriplesPerson(GraphState, Graph::VertexTopologyID);
void matchTriplesForum(GraphState, Graph::VertexTopologyID);
void matchTriplesForumEvent(GraphState, Graph::VertexTopologyID);
void matchTriplesPub(GraphState, Graph::VertexTopologyID);
void calculateSimilarity(State&);
void calculateMatch(State);

} // namespace approx
} // namespace wf2

#endif // PANDO_WF2_GALOIS_APPROX_MATCH_H_
