// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#define DEBUG 0
#if DEBUG == 0
#define DBG_PRINT(x)
#else
#define DBG_PRINT(x) \
  { std::cout << x << std::endl; }
#endif

#include <cmath>

#include "pando-wf2-galois/exact_pattern.h"
#include <pando-lib-galois/containers/hashtable.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/sync/wait_group.hpp>
#include <pando-rt/memory/memory_guard.hpp>

namespace wf2 {
namespace exact {

pando::GlobalPtr<WMDGraph> ImportWMDGraph(std::string& filename) {
  pando::Array<char> filename_arr;
  PANDO_CHECK(filename_arr.initialize(filename.size()));
  for (uint64_t i = 0; i < filename.size(); i++)
    filename_arr[i] = filename[i];

  WMDGraph graph;
  pando::GlobalPtr<WMDGraph> graph_ptr = static_cast<decltype(graph_ptr)>(
      pando::getDefaultMainMemoryResource()->allocate(sizeof(WMDGraph)));
  // bool isEdgeList = false;
  // PANDO_CHECK(graph_ptr->initializeWMD(filename_arr, isEdgeList));

  return graph_ptr;
}

bool proximity_to_nyc(const wf::TopicVertex& A) {
  std::pair<double, double> nyc_loc(40.67, -73.94);
  double lat_miles = 1.15 * std::abs(nyc_loc.first - A.lat);
  double lon_miles = 0.91 * std::abs(nyc_loc.second - A.lon);
  double distance = std::sqrt(lon_miles * lon_miles + lat_miles * lat_miles);
  return distance <= 30.0;
}

void eeTopicMatch(EEState& state, Graph::EdgeHandle eh) {
  Graph graph = state.g;
  wf2::exact::Edge edge = graph.getEdgeData(eh);
  Graph::VertexTopologyID lid = graph.getEdgeDst(eh);
  wf2::exact::Vertex node = graph.getData(lid);

  pando::GlobalRef<bool> matched_ref = state.ee_topic_vec[graph.getVertexIndex(lid)];
  if (edge.type == agile::TYPES::HASTOPIC && graph.getTokenID(lid) == 43035) {
    matched_ref = true;
    DBG_PRINT("Subpattern: EE Topic ");
  }
}

void eeOrgMatch(EEState& state, Graph::EdgeHandle eh) {
  Graph graph = state.g;
  wf2::exact::Edge edge = graph.getEdgeData(eh);
  Graph::VertexTopologyID lid = graph.getEdgeDst(eh);
  wf2::exact::Vertex node = graph.getData(lid);

  pando::GlobalRef<bool> matched_ref = state.ee_org_vec[graph.getVertexIndex(lid)];
  if (edge.type == agile::TYPES::HASORG && proximity_to_nyc(node.v.topic)) {
    matched_ref = true;
    DBG_PRINT("Subpattern: EE NYC Org ");
  }
}

void eePublicationMatch(EEState& state, Graph::EdgeHandle eh) {
  Graph graph = state.g;
  wf2::exact::Edge edge = graph.getEdgeData(eh);
  Graph::VertexTopologyID lid = graph.getEdgeDst(eh);
  wf2::exact::Vertex node = graph.getData(lid);

  if (edge.type == agile::TYPES::AUTHOR && node.type == agile::TYPES::PUBLICATION) {
    for (Graph::EdgeHandle eh : graph.edges(lid)) {
      eeOrgMatch(state, eh);
    }
    for (Graph::EdgeHandle eh : graph.edges(lid)) {
      eeTopicMatch(state, eh);
    }

    bool nyc_org = false;
    bool matched_topic = false;

    for (auto eh : graph.edges(lid)) {
      Graph::VertexTopologyID dst = graph.getEdgeDst(eh);
      if (state.ee_org_vec[graph.getVertexIndex(dst)]) {
        nyc_org = true;
      }
      if (state.ee_topic_vec[graph.getVertexIndex(dst)]) {
        matched_topic = true;
      }
    }
    if (nyc_org && matched_topic) {
      pando::GlobalRef<bool> matched_ref = state.ee_pub_vec[graph.getVertexIndex(lid)];
      matched_ref = true;
      DBG_PRINT("Subpattern: EE Publication");
    }
  }
}

void eeSellerMatch(EEState& state, Graph::EdgeHandle eh) {
  Graph graph = state.g;
  wf2::exact::Edge edge = graph.getEdgeData(eh);
  Graph::VertexTopologyID lid = graph.getEdgeDst(eh);

  if (edge.type == agile::TYPES::PURCHASE && edge.e.sale.product == 11650) {
    for (Graph::EdgeHandle eh : graph.edges(lid)) {
      eePublicationMatch(state, eh);
    }

    for (auto eh : graph.edges(lid)) {
      Graph::VertexTopologyID dst = graph.getEdgeDst(eh);
      if (state.ee_pub_vec[graph.getVertexIndex(dst)]) {
        pando::GlobalRef<bool> matched_ref = state.ee_seller_vec[graph.getVertexIndex(lid)];
        matched_ref = true;
        DBG_PRINT("Subpattern: EE Seller");
        break;
      }
    }
  }
}

void eeMatch(EEState& state, Graph::EdgeHandle eh) {
  Graph graph = state.g;
  wf2::exact::Edge edge = graph.getEdgeData(eh);
  Graph::VertexTopologyID lid = graph.getEdgeDst(eh);
  wf2::exact::Vertex node = graph.getData(lid);

  if (edge.type == agile::TYPES::SALE && node.type == agile::TYPES::PERSON) {
    for (auto eh : graph.edges(lid)) {
      eeSellerMatch(state, eh);
    }
    for (auto eh : graph.edges(lid)) {
      wf2::exact::Edge edge = graph.getEdgeData(eh);
      if (edge.type == agile::TYPES::PURCHASE && edge.e.sale.product == 11650) {
        Graph::VertexTopologyID dst = graph.getEdgeDst(eh);
        if (state.ee_seller_vec[graph.getVertexIndex(dst)]) {
          pando::GlobalRef<bool> matched_ref = state.ee_vec[graph.getVertexIndex(lid)];
          matched_ref = true;
          break;
        }
      }
    }
  }
}

void ammoMatch(PurchaseState& state, Graph::EdgeHandle eh) {
  Graph graph = state.g;
  wf2::exact::Edge edge = graph.getEdgeData(eh);
  Graph::VertexTopologyID lid = graph.getEdgeDst(eh);
  wf2::exact::Vertex node = graph.getData(lid);

  if (edge.type == agile::TYPES::PURCHASE && node.type == agile::TYPES::PERSON) {
    bool sold_ammo = false;
    std::uint64_t purchased = 0;
    for (auto eh : graph.edges(lid)) {
      wf2::exact::Edge edge = graph.getEdgeData(eh);
      if (edge.type == agile::TYPES::SALE && edge.e.sale.product == 185785) {
        Graph::VertexTopologyID dst = graph.getEdgeDst(eh);
        wf2::exact::Vertex dst_node = graph.getData(dst);
        if (sold_ammo && graph.getTokenID(dst) != purchased) {
          pando::GlobalRef<bool> ammo_ref = state.ammo_vec[graph.getVertexIndex(lid)];
          ammo_ref = true;
          DBG_PRINT("Subpattern: Ammo Seller");
          return;
        }
        sold_ammo = true;
        purchased = graph.getTokenID(dst);
      }
    }
  }
}

void purchaseMatch(PurchaseState& state, Graph::VertexTopologyID lid) {
  Graph graph = state.g;
  wf2::exact::Vertex node = graph.getData(lid);

  // TODO(KavyaVarma) Remove the token=0 check
  if (graph.getTokenID(lid) != 0 && node.type == agile::TYPES::PERSON) {
    bool purchase_ee = false;
    std::time_t latest_bb = 0, latest_pc = 0, latest_ammo = 0;

    for (auto eh : graph.edges(lid)) {
      wf2::exact::Edge edge = graph.getEdgeData(eh);
      Graph::VertexTopologyID dst = graph.getEdgeDst(eh);
      if (edge.type == agile::TYPES::PURCHASE && edge.e.sale.product == 271997) {
        latest_pc = std::max(latest_pc, edge.e.sale.date);
      }
      if (edge.type == agile::TYPES::PURCHASE && edge.e.sale.product == 2869238) {
        latest_bb = std::max(latest_bb, edge.e.sale.date);
      }
      if (edge.type == agile::TYPES::PURCHASE && edge.e.sale.product == 11650) {
        eeSellerMatch(state.ee_state, eh);
        if (state.ee_state.ee_seller_vec[graph.getVertexIndex(dst)]) {
          purchase_ee = true;
          DBG_PRINT("Subpattern: EE");
        }
      }
      if (edge.type == agile::TYPES::PURCHASE && edge.e.sale.product == 185785) {
        ammoMatch(state, eh);
        if (state.ammo_vec[graph.getVertexIndex(dst)]) {
          latest_ammo = std::max(latest_ammo, edge.e.sale.date);
        }
      }
    }
    std::time_t trans_date = std::min(std::min(latest_bb, latest_pc), latest_ammo);
    if (purchase_ee && trans_date != 0) {
      if (forumSubPattern(state.forum_state, lid, trans_date))
        std::cout << "Found person: " << graph.getTokenID(lid) << "!!\n";
    }
  }
  return;
}

void forumDateMatch(ForumState& state, Graph::VertexTopologyID lid) {
  Graph graph = state.g;
  wf2::exact::Vertex node = graph.getData(lid);

  if (node.type == agile::TYPES::FORUM) {
    for (Graph::EdgeHandle eh : graph.edges(lid)) {
      forumFE2AMatch(state, eh);
    }
    for (Graph::EdgeHandle eh : graph.edges(lid)) {
      forumFE2BMatch(state, eh);
    }

    bool forum_2a = false;
    std::time_t min_date = false;
    for (auto eh : graph.edges(lid)) {
      Graph::VertexTopologyID dst = graph.getEdgeDst(eh);
      if (state.two_a[graph.getVertexIndex(dst)]) {
        forum_2a = true;
      }
      if (state.two_b[graph.getVertexIndex(dst)]) {
        wf2::exact::Vertex dst_node = graph.getData(dst);
        std::time_t curr_date = dst_node.v.forum_event.date;
        min_date = min_date == 0 ? curr_date : std::min(min_date, curr_date);
      }
    }
    if (forum_2a) {
      pando::GlobalRef<std::time_t> date_ref = state.forum_min_time[graph.getVertexIndex(lid)];
      date_ref = min_date;
      DBG_PRINT("Subpattern: 2a");
    }
  }
}

void forumNYCMatch(ForumState& state, Graph::EdgeHandle eh) {
  Graph graph = state.g;
  wf2::exact::Edge edge = graph.getEdgeData(eh);
  Graph::VertexTopologyID lid = graph.getEdgeDst(eh);
  wf2::exact::Vertex node = graph.getData(lid);

  if (edge.type == agile::TYPES::INCLUDEDIN && node.type == agile::TYPES::FORUM) {
    // galois::doAll(state, graph.edges(lid), &forumNYCTopicMatch);
    for (auto eh : graph.edges(lid)) {
      forumNYCTopicMatch(state, eh);
    }
    for (auto eh : graph.edges(lid)) {
      Graph::VertexTopologyID dst = graph.getEdgeDst(eh);
      if (state.nyc[graph.getVertexIndex(dst)]) {
        pando::GlobalRef<bool> nyc_ref = state.nyc[graph.getVertexIndex(lid)];
        nyc_ref = true;
        DBG_PRINT("Subpattern: Forum + NYC Topic");
      }
    }
  }
}

void forumNYCTopicMatch(ForumState& state, Graph::EdgeHandle eh) {
  Graph graph = state.g;
  wf2::exact::Edge edge = graph.getEdgeData(eh);
  Graph::VertexTopologyID lid = graph.getEdgeDst(eh);
  wf2::exact::Vertex node = graph.getData(lid);

  if (edge.type == agile::TYPES::HASTOPIC && graph.getTokenID(lid) == 60) {
    pando::GlobalRef<bool> nyc_ref = state.nyc[graph.getVertexIndex(lid)];
    nyc_ref = true;
    DBG_PRINT("Subpattern: Forum State + NYC Topic");
  }
}

void forumFE2AMatch(ForumState& state, Graph::EdgeHandle eh) {
  Graph graph = state.g;
  wf2::exact::Edge edge = graph.getEdgeData(eh);
  Graph::VertexTopologyID lid = graph.getEdgeDst(eh);
  wf2::exact::Vertex node = graph.getData(lid);

  if (edge.type == agile::TYPES::INCLUDES && node.type == agile::TYPES::FORUMEVENT) {
    for (Graph::EdgeHandle eh : graph.edges(lid)) {
      forumFE2ATopicMatch(state.fe_2a_state, eh);
    }
    bool outdoors = false, prospect_park = false;
    for (auto eh : graph.edges(lid)) {
      Graph::VertexTopologyID dst = graph.getEdgeDst(eh);
      if (state.fe_2a_state.outdoors[graph.getVertexIndex(dst)])
        outdoors = true;
      if (state.fe_2a_state.prospect_park[graph.getVertexIndex(dst)])
        prospect_park = true;
    }
    if (outdoors && prospect_park) {
      pando::GlobalRef<bool> two_a_ref = state.two_a[graph.getVertexIndex(lid)];
      two_a_ref = true;
    }
  }
}

void forumFE2ATopicMatch(ForumEvent2AState& state, Graph::EdgeHandle eh) {
  Graph graph = state.g;
  wf2::exact::Edge edge = graph.getEdgeData(eh);
  Graph::VertexTopologyID lid = graph.getEdgeDst(eh);
  wf2::exact::Vertex node = graph.getData(lid);

  pando::GlobalRef<bool> outdoors_ref = state.outdoors[graph.getVertexIndex(lid)];
  pando::GlobalRef<bool> prospect_park_ref = state.prospect_park[graph.getVertexIndex(lid)];
  if (edge.type == agile::TYPES::HASTOPIC && graph.getTokenID(lid) == 69871376)
    outdoors_ref = true;
  else if (edge.type == agile::TYPES::HASTOPIC && graph.getTokenID(lid) == 1049632)
    prospect_park_ref = true;
}

void forumFE2BMatch(ForumState& state, Graph::EdgeHandle eh) {
  Graph graph = state.g;
  wf2::exact::Edge edge = graph.getEdgeData(eh);
  Graph::VertexTopologyID lid = graph.getEdgeDst(eh);
  wf2::exact::Vertex node = graph.getData(lid);

  if (edge.type == agile::TYPES::INCLUDES && node.type == agile::TYPES::FORUMEVENT) {
    for (Graph::EdgeHandle eh : graph.edges(lid)) {
      forumFE2BTopicMatch(state.fe_2b_state, eh);
    }
    bool williamsburg = false, explosion = false, bomb = false;
    for (auto eh : graph.edges(lid)) {
      Graph::VertexTopologyID dst = graph.getEdgeDst(eh);
      if (state.fe_2b_state.williamsburg[graph.getVertexIndex(dst)])
        williamsburg = true;
      if (state.fe_2b_state.explosion[graph.getVertexIndex(dst)])
        explosion = true;
      if (state.fe_2b_state.bomb[graph.getVertexIndex(dst)])
        bomb = true;
    }
    if (williamsburg && explosion && bomb) {
      pando::GlobalRef<bool> two_b_ref = state.two_b[graph.getVertexIndex(lid)];
      two_b_ref = true;
    }
  }
}

void forumFE2BTopicMatch(ForumEvent2BState& state, Graph::EdgeHandle eh) {
  Graph graph = state.g;
  wf2::exact::Edge edge = graph.getEdgeData(eh);
  Graph::VertexTopologyID lid = graph.getEdgeDst(eh);
  wf2::exact::Vertex node = graph.getData(lid);

  pando::GlobalRef<bool> williamsburg_ref = state.williamsburg[graph.getVertexIndex(lid)];
  pando::GlobalRef<bool> explosion_ref = state.explosion[graph.getVertexIndex(lid)];
  pando::GlobalRef<bool> bomb_ref = state.bomb[graph.getVertexIndex(lid)];

  if (edge.type == agile::TYPES::HASTOPIC && graph.getTokenID(lid) == 771572)
    williamsburg_ref = true;
  else if (edge.type == agile::TYPES::HASTOPIC && graph.getTokenID(lid) == 179057)
    explosion_ref = true;
  else if (edge.type == agile::TYPES::HASTOPIC && graph.getTokenID(lid) == 127197)
    bomb_ref = true;
}

void forumFEJihadMatch(ForumState& state, Graph::EdgeHandle eh) {
  Graph graph = state.g;
  Graph::VertexTopologyID lid = graph.getEdgeDst(eh);
  wf2::exact::Vertex node = graph.getData(lid);

  if (node.type == agile::TYPES::FORUMEVENT) {
    bool jihad = false;
    for (auto eh : graph.edges(lid)) {
      forumFEJihadTopicMatch(state, eh);
      Graph::VertexTopologyID dst = graph.getEdgeDst(eh);
      if (state.jihad[graph.getVertexIndex(dst)])
        jihad = true;
    }
    if (jihad) {
      pando::GlobalRef<bool> jihad_ref = state.jihad[graph.getVertexIndex(lid)];
      jihad_ref = true;
    }
  }
}

void forumFEJihadTopicMatch(ForumState& state, Graph::EdgeHandle eh) {
  Graph graph = state.g;
  wf2::exact::Edge edge = graph.getEdgeData(eh);
  Graph::VertexTopologyID lid = graph.getEdgeDst(eh);
  wf2::exact::Vertex node = graph.getData(lid);

  pando::GlobalRef<bool> jihad_ref = state.jihad[graph.getVertexIndex(lid)];

  if (edge.type == agile::TYPES::HASTOPIC && graph.getTokenID(lid) == 44311) {
    jihad_ref = true;
  }
}

bool forumSubPattern(ForumState& state, Graph::VertexTopologyID lid, std::time_t trans_date) {
  Graph graph = state.g;
  wf2::exact::Vertex node = graph.getData(lid);

  if (node.type == agile::TYPES::PERSON) {
    forum1(state, lid);
    forum2(state, lid, trans_date);
  }
  return state.forum1[graph.getVertexIndex(lid)] && state.forum2[graph.getVertexIndex(lid)];
}

void forum1(ForumState& state, Graph::VertexTopologyID lid) {
  Graph graph = state.g;
  wf2::exact::Vertex node = graph.getData(lid);
  galois::HashTable<std::uint64_t, std::uint64_t> forums;
  forums.initialize(1);
  pando::GlobalRef<bool> forum1_ref = state.forum1[graph.getVertexIndex(lid)];
  if (node.type == agile::TYPES::PERSON) {
    for (auto eh : graph.edges(lid)) {
      wf2::exact::Edge edge = graph.getEdgeData(eh);
      Graph::VertexTopologyID dst = graph.getEdgeDst(eh);
      wf2::exact::Vertex dst_node = graph.getData(dst);

      if (edge.type == agile::TYPES::AUTHOR && dst_node.type == agile::TYPES::FORUMEVENT) {
        forumFEJihadMatch(state, eh);
        if (state.jihad[graph.getVertexIndex(dst)]) {
          for (auto eh : graph.edges(dst)) {
            forumNYCMatch(state, eh);
          }
          for (auto dst_eh : graph.edges(dst)) {
            wf2::exact::Edge dst_edge = graph.getEdgeData(dst_eh);
            Graph::VertexTopologyID forum_lid = graph.getEdgeDst(dst_eh);
            if (dst_edge.type == agile::TYPES::INCLUDEDIN &&
                state.nyc[graph.getVertexIndex(graph.getEdgeDst(dst_eh))]) {
              std::uint64_t count = 0;
              forums.get(graph.getVertexIndex(forum_lid), count);
              if (count >= 1) {
                forum1_ref = true;
                forums.deinitialize();
                return;
              }
              forums.put(graph.getVertexIndex(forum_lid), count + 1);
              break;
            }
          }
        }
      }
    }
  }
  forums.deinitialize();
}

void forum2(ForumState& state, Graph::VertexTopologyID lid, std::time_t trans_date) {
  Graph graph = state.g;
  wf2::exact::Vertex node = graph.getData(lid);
  pando::GlobalRef<bool> forum2_ref = state.forum2[graph.getVertexIndex(lid)];
  if (node.type == agile::TYPES::PERSON) {
    for (auto eh : graph.edges(lid)) {
      wf2::exact::Edge edge = graph.getEdgeData(eh);
      Graph::VertexTopologyID dst = graph.getEdgeDst(eh);
      wf2::exact::Vertex dst_node = graph.getData(dst);

      if (edge.type == agile::TYPES::AUTHOR && dst_node.type == agile::TYPES::FORUMEVENT) {
        for (auto dst_eh : graph.edges(dst)) {
          wf2::exact::Edge dst_edge = graph.getEdgeData(dst_eh);
          Graph::VertexTopologyID forum_lid = graph.getEdgeDst(dst_eh);
          if (dst_edge.type == agile::TYPES::INCLUDEDIN) {
            std::time_t date_ref = state.forum_min_time[graph.getVertexIndex(forum_lid)];
            if ((date_ref > 0) && (date_ref < trans_date)) {
              forum2_ref = true;
              return;
            }
            break;
          }
        }
      }
    }
  }
}

void patternMatch(pando::GlobalPtr<Graph> graph_ptr) {
  DBG_PRINT("Starting pattern match");
  Graph graph = *graph_ptr;
  DBG_PRINT("Graph Size: " << graph.size());
  uint64_t num_nodes = graph.size();

  PurchaseState purchase_state(graph);
  purchase_state.initialize(num_nodes);

  DBG_PRINT("Initialized state");
  galois::doAll(purchase_state.forum_state, graph.vertices(), &forumDateMatch);
  DBG_PRINT("Forum Date Match Done");
  galois::doAll(purchase_state, graph.vertices(), &purchaseMatch);
  DBG_PRINT("Finished Pattern Match");
}

} // namespace exact
} // namespace wf2
