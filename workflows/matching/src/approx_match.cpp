// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.
//
#include <cmath>

#include "pando-wf2-galois/approx_match.h"
#include <pando-lib-galois/sorts/merge_sort.hpp>

#define DEBUG 0
#if DEBUG == 0
#define DBG_PRINT(x)
#else
#define DBG_PRINT(x) \
  { x }
#endif

#define COSINE_COMPUTE(bitmap)                                                                    \
  cosine_compute(state.state_lhs.bitmap[state.state_lhs.graph.getVertexIndex(lhs_lid)],           \
                 state.state_rhs.bitmap[state.state_rhs.graph.getVertexIndex(rhs_lid)], adj, dot, \
                 lenVA, lenVB);

namespace wf2 {
namespace approx {

bool edge_comp(std::pair<std::uint64_t, double> a, std::pair<std::uint64_t, double> b) {
  return a.second < b.second;
}
bool node_sim_comp(node_sim a, node_sim b) {
  return a.similarity == b.similarity ? a.token < b.token : a.similarity < b.similarity;
}

void cosine_compute(std::uint64_t triple_a, std::uint64_t triple_b, double& adj, double& dot,
                    double& lenVA, double& lenVB) {
  double minTriple = std::min(triple_a, triple_b);
  adj += minTriple * minTriple;
  dot += triple_a * triple_b;
  lenVA += triple_a * triple_a;
  lenVB += triple_b * triple_b;
}

double cosine_similarity(State& state, Graph::VertexTopologyID lhs_lid,
                         Graph::VertexTopologyID rhs_lid) {
  double similarity = 0.0;
  double adj = 0.0, dot = 0.0, lenVA = 0.0, lenVB = 0.0;

  COSINE_COMPUTE(person_sale_person_bomb_bath);
  COSINE_COMPUTE(person_sale_person_pressure_cooker);
  COSINE_COMPUTE(person_sale_person_ammunition);
  COSINE_COMPUTE(person_sale_person_electronics);
  COSINE_COMPUTE(person_purchase_person_bomb_bath);
  COSINE_COMPUTE(person_purchase_person_pressure_cooker);
  COSINE_COMPUTE(person_purchase_person_ammunition);
  COSINE_COMPUTE(person_purchase_person_electronics);
  COSINE_COMPUTE(person_author_forumevent);
  COSINE_COMPUTE(person_author_publication);
  COSINE_COMPUTE(forum_includes_forumevent);
  COSINE_COMPUTE(forum_hastopic_topic_nyc);
  COSINE_COMPUTE(forumevent_hastopic_topic_bomb);
  COSINE_COMPUTE(forumevent_hastopic_topic_explosion);
  COSINE_COMPUTE(forumevent_hastopic_topic_williamsburg);
  COSINE_COMPUTE(forumevent_hastopic_topic_outdoors);
  COSINE_COMPUTE(forumevent_hastopic_topic_prospect_park);
  COSINE_COMPUTE(forumevent_hastopic_topic_jihad);
  COSINE_COMPUTE(publication_hasorg_topic_near_nyc);
  COSINE_COMPUTE(publication_hastopic_topic_electrical_eng);

  if ((lenVA != 0.0) && (lenVB != 0.0))
    similarity = (sqrt(adj) * dot) / (sqrt(lenVA) * sqrt(lenVB));

  return similarity;
}

std::uint64_t partitionArray(pando::Vector<node_sim> arr) {
  std::uint64_t index = 0;
  for (std::uint64_t i = 0; i < arr.size(); i++) {
    node_sim arr_i = arr[i];
    if (arr_i.similarity != 0) {
      arr[index] = arr_i;
      index++;
    }
  }
  node_sim temp(0, 0, 0);
  for (std::uint64_t i = index; i < arr.size(); i++) {
    arr[i] = temp;
  }
  return index;
}

void match(pando::GlobalPtr<Graph> lhs_ptr, pando::GlobalPtr<Graph> rhs_ptr, int k) {
  DBG_PRINT(std::cout << "approximate matching\n";)
  Graph lhs = *(lhs_ptr);
  Graph rhs = *(rhs_ptr);
  State state(lhs, rhs);
  state.initialize();
  DBG_PRINT(std::cout << "State Initialized\n";)
  galois::doAll(state.state_lhs, lhs.vertices(), &matchTriples);
  galois::doAll(state.state_rhs, rhs.vertices(), &matchTriples);
  DBG_PRINT(std::cout << "Matched Triples\n";)
  calculateSimilarity(state);
  DBG_PRINT(std::cout << "Calculated Similarity\n";)
  galois::doAll(
      state.state_rhs.similarity, +[](pando::Vector<node_sim> arr) {
        std::uint64_t index = partitionArray(arr);
        galois::merge_sort_n<node_sim>(arr, &node_sim_comp, index);
      });
  DBG_PRINT(std::cout << "Sorted RHS\n";)
  galois::doAll(
      state.state_lhs.similarity, +[](pando::Vector<node_sim> arr) {
        std::uint64_t index = partitionArray(arr);
        galois::merge_sort_n<node_sim>(arr, &node_sim_comp, index);
      });
  DBG_PRINT(std::cout << "Sorted LHS\n";)
  for (int i = 0; i < k; i++)
    calculateMatch(state);
  DBG_PRINT(std::cout << "Calculated Match\n";)
}

bool proximity(const wf::TopicVertex& A) {
  std::pair<double, double> nyc_loc(40.67, -73.94);
  double lat_miles = 1.15 * std::abs(nyc_loc.first - A.lat);
  double lon_miles = 0.91 * std::abs(nyc_loc.second - A.lon);
  double distance = std::sqrt(lon_miles * lon_miles + lat_miles * lat_miles);
  return distance <= 30.0;
}

void matchTriples(GraphState state, Graph::VertexTopologyID lid) {
  Graph graph = state.graph;
  wf2::approx::Vertex node = graph.getData(lid);

  switch (node.type) {
    case agile::TYPES::PERSON:
      matchTriplesPerson(state, lid);
      break;
    case agile::TYPES::FORUMEVENT:
      matchTriplesForumEvent(state, lid);
      break;
    case agile::TYPES::FORUM:
      matchTriplesForum(state, lid);
      break;
    case agile::TYPES::PUBLICATION:
      matchTriplesPub(state, lid);
      break;
    default:
      break;
  }
}

void matchTriplesPerson(GraphState state, Graph::VertexTopologyID lid) {
  Graph graph = state.graph;
  for (auto e : graph.edges(lid)) {
    wf2::approx::Edge edge = graph.getEdgeData(e);

    switch (edge.type) {
      case agile::TYPES::SALE:
        if (edge.e.sale.product == 2869238) {
          state.person_sale_person_bomb_bath[graph.getVertexIndex(lid)]++;
        } else if (edge.e.sale.product == 271997) {
          state.person_sale_person_pressure_cooker[graph.getVertexIndex(lid)]++;
        } else if (edge.e.sale.product == 185785) {
          state.person_sale_person_ammunition[graph.getVertexIndex(lid)]++;
        } else if (edge.e.sale.product == 11650) {
          state.person_sale_person_electronics[graph.getVertexIndex(lid)]++;
        }
        break;

      case agile::TYPES::PURCHASE:
        if (edge.e.sale.product == 2869238) {
          state.person_purchase_person_bomb_bath[graph.getVertexIndex(lid)]++;
        } else if (edge.e.sale.product == 271997) {
          state.person_purchase_person_pressure_cooker[graph.getVertexIndex(lid)]++;
        } else if (edge.e.sale.product == 185785) {
          state.person_purchase_person_ammunition[graph.getVertexIndex(lid)]++;
        } else if (edge.e.sale.product == 11650) {
          state.person_purchase_person_electronics[graph.getVertexIndex(lid)]++;
        }
        break;

      case agile::TYPES::AUTHOR:
        if (edge.dstType == agile::TYPES::FORUMEVENT) {
          state.person_author_forumevent[graph.getVertexIndex(lid)]++;
        } else if (edge.dstType == agile::TYPES::PUBLICATION) {
          state.person_author_publication[graph.getVertexIndex(lid)]++;
        }
        break;
      default:
        break;
    }
  }
}

void matchTriplesForumEvent(GraphState state, Graph::VertexTopologyID lid) {
  Graph graph = state.graph;
  for (auto e : graph.edges(lid)) {
    wf2::approx::Edge edge = graph.getEdgeData(e);

    switch (edge.type) {
      case agile::TYPES::HASTOPIC:
        if (edge.e.has_topic.topic == 127197) {
          state.forumevent_hastopic_topic_bomb[graph.getVertexIndex(lid)]++;
        } else if (edge.e.has_topic.topic == 179057) {
          state.forumevent_hastopic_topic_explosion[graph.getVertexIndex(lid)]++;
        } else if (edge.e.has_topic.topic == 771572) {
          state.forumevent_hastopic_topic_williamsburg[graph.getVertexIndex(lid)]++;
        } else if (edge.e.has_topic.topic == 1049632) {
          state.forumevent_hastopic_topic_prospect_park[graph.getVertexIndex(lid)]++;
        } else if (edge.e.has_topic.topic == 69871376) {
          state.forumevent_hastopic_topic_outdoors[graph.getVertexIndex(lid)]++;
        } else if (edge.e.has_topic.topic == 44311) {
          state.forumevent_hastopic_topic_jihad[graph.getVertexIndex(lid)]++;
        }
        break;
      default:
        break;
    }
  }
}

void matchTriplesForum(GraphState state, Graph::VertexTopologyID lid) {
  Graph graph = state.graph;
  for (auto e : graph.edges(lid)) {
    wf2::approx::Edge edge = graph.getEdgeData(e);
    switch (edge.type) {
      case agile::TYPES::INCLUDES:
        state.forum_includes_forumevent[graph.getVertexIndex(lid)]++;
        break;
      case agile::TYPES::HASTOPIC:
        if (edge.e.has_topic.topic == 60) {
          state.forum_hastopic_topic_nyc[graph.getVertexIndex(lid)]++;
        }
        break;
      default:
        break;
    }
  }
}

void matchTriplesPub(GraphState state, Graph::VertexTopologyID lid) {
  Graph graph = state.graph;
  for (auto e : graph.edges(lid)) {
    wf2::approx::Edge edge = graph.getEdgeData(e);
    switch (edge.type) {
      case agile::TYPES::HASORG: {
        wf2::approx::Vertex dst_node = graph.getData(graph.getEdgeDst(e));
        if (proximity(dst_node.v.topic)) {
          state.forum_includes_forumevent[graph.getVertexIndex(lid)]++;
        }
      } break;
      case agile::TYPES::HASTOPIC:
        if (edge.e.has_topic.topic == 43035) {
          state.publication_hastopic_topic_electrical_eng[graph.getVertexIndex(lid)]++;
        }
        break;
      default:
        break;
    }
  }
}

void calculateLhsSimilarity(State state, Graph::VertexTopologyID lhs_lid) {
  Graph& graph = state.state_lhs.graph;
  wf2::approx::Vertex lhs_node = graph.getData(lhs_lid);
  pando::Vector<node_sim> lhs_list =
      state.state_lhs.similarity[state.state_lhs.graph.getVertexIndex(lhs_lid)];
  for (Graph::VertexTopologyID rhs_lid : state.state_rhs.graph.vertices()) {
    double sim = 0;
    wf2::approx::Vertex rhs_node = state.state_rhs.graph.getData(rhs_lid);
    if (rhs_node.type == lhs_node.type)
      sim = cosine_similarity(state, lhs_lid, rhs_lid);
    auto lhs_ref = lhs_list[state.state_rhs.graph.getVertexIndex(rhs_lid)];
    lhs_ref = node_sim(state.state_rhs.graph.getVertexIndex(rhs_lid), sim,
                       state.state_rhs.graph.getTokenID(rhs_lid));
    pando::Vector<node_sim> rhs_list =
        state.state_rhs.similarity[state.state_rhs.graph.getVertexIndex(rhs_lid)];
    auto rhs_ref = rhs_list[state.state_lhs.graph.getVertexIndex(lhs_lid)];
    rhs_ref = node_sim(state.state_lhs.graph.getVertexIndex(lhs_lid), sim,
                       state.state_lhs.graph.getTokenID(lhs_lid));
  }
}

void calculateSimilarity(State& state) {
  Graph& lhs = state.state_lhs.graph;

  galois::doAll(state, lhs.vertices(), &calculateLhsSimilarity);
}

void findLhsMatch(State state, Graph::VertexTopologyID lid) {
  if (state.state_lhs.matched[state.state_lhs.graph.getVertexIndex(lid)])
    return;
  pando::Vector<node_sim> lhs_sim =
      state.state_lhs.similarity[state.state_lhs.graph.getVertexIndex(lid)];
  auto matched_ref = state.state_lhs.matched[state.state_lhs.graph.getVertexIndex(lid)];
  auto match_ref = state.state_lhs.match[state.state_lhs.graph.getVertexIndex(lid)];

  for (node_sim sim : lhs_sim) {
    if (sim.similarity == 0) {
      match_ref = node_sim(0, 0, 0);
      matched_ref = true;
      return;
    }
    if (state.state_rhs.matched[state.state_rhs.graph.getVertexIndex(
            state.state_rhs.graph.getTopologyID(sim.token))])
      continue;
    match_ref = sim;
    return;
  }
  match_ref = node_sim(0, 0, 0);
  matched_ref = true;
}

void checkLhsMatch(State state, Graph::VertexTopologyID lid) {
  if (state.state_lhs.matched[state.state_lhs.graph.getVertexIndex(lid)])
    return;
  node_sim lhs_match = state.state_lhs.match[state.state_lhs.graph.getVertexIndex(lid)];
  pando::Vector<node_sim> rhs_sim = state.state_rhs.similarity[state.state_rhs.graph.getVertexIndex(
      state.state_rhs.graph.getTopologyID(lhs_match.token))];

  for (node_sim lhs_node : rhs_sim) {
    if (state.state_lhs.matched[state.state_lhs.graph.getVertexIndex(
            state.state_lhs.graph.getTopologyID(lhs_node.token))])
      continue;
    if (lhs_node.lid == state.state_lhs.graph.getVertexIndex(lid)) {
      auto lhs_matched_ref = state.state_lhs.new_matched[state.state_lhs.graph.getVertexIndex(lid)];
      lhs_matched_ref = true;
      auto rhs_matched_ref = state.state_rhs.matched[state.state_rhs.graph.getVertexIndex(
          state.state_rhs.graph.getTopologyID(lhs_match.token))];
      rhs_matched_ref = true;
      return;
    }
    break;
  }
}

void calculateMatch(State state) {
  for (auto m : state.state_lhs.matched)
    m = 0;
  auto match_count = *(state.match_count_ptr);
  match_count = 0;

  while (true) {
    for (auto m : state.state_lhs.new_matched)
      m = 0;
    galois::doAll(state, state.state_lhs.graph.vertices(), &findLhsMatch);
    galois::doAll(state, state.state_lhs.graph.vertices(), &checkLhsMatch);
    for (auto lid : state.state_lhs.graph.vertices()) {
      if (state.state_lhs.new_matched[state.state_lhs.graph.getVertexIndex(lid)])
        state.state_lhs.matched[state.state_lhs.graph.getVertexIndex(lid)] = true;
    }
    bool all_matched = true;
    for (auto matched : state.state_lhs.matched) {
      if (!matched)
        all_matched = false;
    }
    if (all_matched)
      break;
  }
  std::cout << "********** Match **********\n";
  for (auto lid : state.state_lhs.graph.vertices()) {
    node_sim lhs_match = state.state_lhs.match[state.state_lhs.graph.getVertexIndex(lid)];
    wf2::approx::Vertex lhs_node = state.state_lhs.graph.getData(lid);
    wf2::approx::Graph::VertexTokenID lhs_token = state.state_lhs.graph.getTokenID(lid);
    if (lhs_node.type == agile::TYPES::TOPIC)
      continue;
    if (lhs_match.similarity != 0) {
      std::cout << "Pattern vertex  " << lhs_token << " matched to Data vertex " << lhs_match.token
                << std::endl;
    } else {
      std::cout << "Pattern vertex  " << lhs_token << " matched to Data vertex "
                << " *******" << std::endl;
    }
  }
}

} // namespace approx
} // namespace wf2
