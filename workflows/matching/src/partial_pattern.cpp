// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.
#include <cmath>

#include "pando-wf2-galois/partial_pattern.h"
#include <pando-lib-galois/containers/hashtable.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/sync/wait_group.hpp>
#include <pando-rt/memory/memory_guard.hpp>
#include <pando-rt/sync/atomic.hpp>
#include <pando-rt/sync/notification.hpp>

#define DEBUG_PARTIAL 0
#if DEBUG_PARTIAL == 0
#define DBG_PRINT_PARTIAL(x)
#else
#define DBG_PRINT_PARTIAL(x) \
  { x }
#endif

namespace wf2 {
namespace partial {

bool proximity(const wf::TopicVertex& A) {
  std::pair<double, double> nyc_loc(40.67, -73.94);
  double lat_miles = 1.15 * std::abs(nyc_loc.first - A.lat);
  double lon_miles = 0.91 * std::abs(nyc_loc.second - A.lon);
  double distance = std::sqrt(lon_miles * lon_miles + lat_miles * lat_miles);
  return distance <= 30.0;
}

void atomicAdd(pando::GlobalPtr<std::int64_t> countPtr, std::uint32_t delta) {
  pando::Notification notify;
  PANDO_CHECK(notify.init());
  const auto countLocalityNodePlace =
      pando::Place{pando::localityOf(countPtr).node, pando::anyPod, pando::anyCore};
  pando::atomicThreadFence(std::memory_order_release);
  PANDO_CHECK(pando::executeOn(
      countLocalityNodePlace,
      +[](pando::GlobalPtr<std::int64_t> countPtr, std::uint32_t delta,
          pando::NotificationHandle handle) {
        pando::atomicFetchAdd(countPtr, static_cast<std::int64_t>(delta),
                              std::memory_order_release);
        handle.notify();
      },
      countPtr, static_cast<std::uint32_t>(delta), notify.getHandle()));
  notify.wait();
}

void swapMaxDate(pando::GlobalPtr<std::time_t> date_ptr, std::time_t new_date) {
  pando::Notification notify;
  PANDO_CHECK(notify.init());
  const auto countLocalityNodePlace =
      pando::Place{pando::localityOf(date_ptr).node, pando::anyPod, pando::anyCore};
  PANDO_CHECK(pando::executeOn(
      countLocalityNodePlace,
      +[](pando::GlobalPtr<std::time_t> date_ptr, std::time_t new_date,
          pando::NotificationHandle handle) {
        std::time_t expected = *(date_ptr);
        while (expected < new_date &&
               pando::atomicCompareExchange(date_ptr, expected, new_date) != expected) {
          expected = *(date_ptr);
        }
        handle.notify();
      },
      date_ptr, new_date, notify.getHandle()));
  notify.wait();
}

void swapMinDate(pando::GlobalPtr<std::time_t> date_ptr, std::time_t new_date) {
  pando::Notification notify;
  PANDO_CHECK(notify.init());
  const auto countLocalityNodePlace =
      pando::Place{pando::localityOf(date_ptr).node, pando::anyPod, pando::anyCore};
  PANDO_CHECK(pando::executeOn(
      countLocalityNodePlace,
      +[](pando::GlobalPtr<std::time_t> date_ptr, std::time_t new_date,
          pando::NotificationHandle handle) {
        std::time_t expected = *(date_ptr);
        while ((expected == 0 || expected > new_date) &&
               pando::atomicCompareExchange(date_ptr, expected, new_date) != expected) {
          expected = *(date_ptr);
        }
        handle.notify();
      },
      date_ptr, new_date, notify.getHandle()));
  notify.wait();
}

template <typename T>
void swapValue(pando::GlobalPtr<T> val_ptr, T new_val) {
  pando::Notification notify;
  PANDO_CHECK(notify.init());
  const auto countLocalityNodePlace =
      pando::Place{pando::localityOf(val_ptr).node, pando::anyPod, pando::anyCore};
  PANDO_CHECK(pando::executeOn(
      countLocalityNodePlace,
      +[](pando::GlobalPtr<T> val_ptr, T new_val, pando::NotificationHandle handle) {
        T expected = *(val_ptr);
        while (expected == 0 &&
               pando::atomicCompareExchange(val_ptr, expected, new_val) != expected) {
          expected = *(val_ptr);
        }
        handle.notify();
      },
      val_ptr, new_val, notify.getHandle()));
  notify.wait();
}

void matchForum1(State state, Graph::EdgeHandle eh, Graph::VertexTopologyID lid,
                 galois::WaitGroup::HandleType wgh) {
  wf2::partial::Edge edge = state.graph.getEdgeData(eh);
  Graph::VertexTopologyID dst = state.graph.getEdgeDst(eh);
  wf2::partial::Vertex node = state.graph.getData(dst);
  if (edge.type == agile::TYPES::AUTHOR && node.type == agile::TYPES::FORUMEVENT) {
    for (auto dst_eh : state.graph.edges(dst)) {
      wf2::partial::Edge dst_edge = state.graph.getEdgeData(dst_eh);
      Graph::VertexTopologyID forum_dst = state.graph.getEdgeDst(dst_eh);
      wf2::partial::Vertex forum_node = state.graph.getData(forum_dst);

      if (dst_edge.type == agile::TYPES::INCLUDEDIN && forum_node.type == agile::TYPES::FORUM) {
        if (state.sp12[state.graph.getVertexIndex(forum_dst)] &&
            state.forum_date[state.graph.getVertexIndex(forum_dst)] != 0 &&
            state.forum_date[state.graph.getVertexIndex(forum_dst)] <
                state.trans_date[state.graph.getVertexIndex(lid)]) {
          pando::GlobalRef<bool> forum1_ref = state.forum1[state.graph.getVertexIndex(lid)];
          forum1_ref = true;
          DBG_PRINT_PARTIAL(std::cout << lid << " forum 1 found\n";)
        }
        break;
      }
    }
  }
  wgh.done();
}

void matchForum2(State& state, Graph::VertexTopologyID lid) {
  galois::HashTable<std::uint64_t, std::uint64_t> forums;
  forums.initialize(1);
  bool found = false;
  for (auto eh : state.graph.edges(lid)) {
    wf2::partial::Edge edge = state.graph.getEdgeData(eh);
    Graph::VertexTopologyID dst = state.graph.getEdgeDst(eh);
    wf2::partial::Vertex dst_node = state.graph.getData(dst);
    if (edge.type == agile::TYPES::AUTHOR && dst_node.type == agile::TYPES::FORUMEVENT &&
        state.jihad[state.graph.getVertexIndex(dst)]) {
      for (auto dst_eh : state.graph.edges(dst)) {
        wf2::partial::Edge dst_edge = state.graph.getEdgeData(dst_eh);
        Graph::VertexTopologyID forum_dst = state.graph.getEdgeDst(dst_eh);
        wf2::partial::Vertex forum_node = state.graph.getData(forum_dst);

        if (dst_edge.type == agile::TYPES::INCLUDEDIN && forum_node.type == agile::TYPES::FORUM) {
          if (state.nyc[state.graph.getVertexIndex(forum_dst)]) {
            std::uint64_t count = 0;
            forums.get(state.graph.getVertexIndex(forum_dst), count);
            if (count > 0) {
              found = true;
              pando::GlobalRef<bool> forum2_ref = state.forum2[state.graph.getVertexIndex(lid)];
              forum2_ref = true;
              DBG_PRINT_PARTIAL(std::cout << lid << " forum 2 found\n";)
            }
            forums.put(state.graph.getVertexIndex(forum_dst), count + 1);
          }
          break;
        }
        if (found)
          break;
      }
    }
  }
  forums.deinitialize();
}

void patternCheck(State state, Graph::VertexTopologyID lid) {
  if (state.interesting_persons[state.graph.getVertexIndex(lid)]) {
    galois::WaitGroup wg;
    PANDO_CHECK(wg.initialize(state.graph.edges(lid).size()));
    for (Graph::EdgeHandle eh : state.graph.edges(lid)) {
      const auto eh_node_place =
          pando::Place{pando::localityOf(eh).node, pando::anyPod, pando::anyCore};
      PANDO_CHECK(pando::executeOn(eh_node_place, &matchForum1, state, eh, lid, wg.getHandle()));
    }
    matchForum2(state, lid);
    PANDO_CHECK(wg.wait());
    if (state.forum1[state.graph.getVertexIndex(lid)] &&
        state.forum2[state.graph.getVertexIndex(lid)]) {
      wf2::partial::Vertex node = state.graph.getData(lid);
      if (!state.matched_persons[state.graph.getVertexIndex(lid)])
        std::cout << state.graph.getTokenID(lid) << ": Person Matched \n";
      state.matched_persons[state.graph.getVertexIndex(lid)] = true;
    }
  }
}

void matchFe2A2B(State state, Graph::VertexTopologyID lid) {
  pando::GlobalRef<bool> ref1 = state.f2a[state.graph.getVertexIndex(lid)];
  pando::GlobalRef<bool> ref2 = state.f2b[state.graph.getVertexIndex(lid)];

  if (ref1 && ref2) {
    DBG_PRINT_PARTIAL(std::cout << " " << state.graph.getVertexIndex(lid) << " 2a2b!!\n";)
    pando::GlobalRef<bool> ref = state.sp12[state.graph.getVertexIndex(lid)];
    ref = true;
    galois::doAll(state, state.graph.vertices(), &patternCheck);
  }
}

void matchPurchases(State state, Graph::VertexTopologyID lid) {
  pando::GlobalRef<bool> ref1 = state.purchase_pc[state.graph.getVertexIndex(lid)];
  pando::GlobalRef<bool> ref2 = state.purchase_bb[state.graph.getVertexIndex(lid)];
  pando::GlobalRef<bool> ref3 = state.purchase_ee[state.graph.getVertexIndex(lid)];
  pando::GlobalRef<bool> ref4 = state.purchase_ammo[state.graph.getVertexIndex(lid)];

  if (ref1 && ref2 && ref3 && ref4) {
    pando::GlobalRef<bool> ref = state.interesting_persons[state.graph.getVertexIndex(lid)];
    ref = true;
    patternCheck(state, lid);
  }
}

// Expects an edge of the form PERSON -- PURCHASE -> PERSON
// Checks if the seller of ammunition is a distributor i.e. has sold ammunition to over 2 buyers
void matchAmmoPurchase(State& state, Graph::VertexTopologyID& lid, Graph::EdgeHandle& eh) {
  wf2::partial::Edge edge = state.graph.getEdgeData(eh);
  Graph::VertexTopologyID dst = state.graph.getEdgeDst(eh);
  wf2::partial::Vertex node = state.graph.getData(lid);

  pando::GlobalRef<bool> ammo_dist_ref = state.ammo_dist[state.graph.getVertexIndex(dst)];
  if (ammo_dist_ref) {
    pando::GlobalRef<bool> purchase_ref = state.purchase_ammo[state.graph.getVertexIndex(lid)];
    purchase_ref = true;
    swapMaxDate(&(state.trans_date[state.graph.getVertexIndex(lid)]), edge.e.sale.date);
    PANDO_CHECK(pando::executeOn(pando::getCurrentPlace(), &matchPurchases, state, lid));
    return;
  }

  pando::GlobalRef<bool> seller_ref = state.ammo_seller[state.graph.getVertexIndex(dst)];
  pando::GlobalRef<std::uint64_t> buyer_ref = state.ammo_buyer[state.graph.getVertexIndex(dst)];
  if (!seller_ref) {
    swapValue(&buyer_ref, state.graph.getTokenID(lid));
    seller_ref = true;
  }
  if (buyer_ref != state.graph.getTokenID(lid)) {
    ammo_dist_ref = true;
    for (auto buyer_eh : state.graph.edges(dst)) {
      wf2::partial::Edge buyer_edge = state.graph.getEdgeData(buyer_eh);
      Graph::VertexTopologyID buyer_dst = state.graph.getEdgeDst(buyer_eh);
      wf2::partial::Vertex buyer_node = state.graph.getData(buyer_dst);

      if (buyer_edge.type == agile::TYPES::SALE && buyer_node.type == agile::TYPES::PERSON &&
          buyer_edge.e.sale.product == 185785) {
        pando::GlobalRef<bool> buyer_ref =
            state.purchase_ammo[state.graph.getVertexIndex(buyer_dst)];
        DBG_PRINT_PARTIAL(std::cout << buyer_dst << " ammo dist\n";)
        buyer_ref = true;
        swapMaxDate(&(state.trans_date[state.graph.getVertexIndex(buyer_dst)]),
                    buyer_edge.e.sale.date);
        PANDO_CHECK(pando::executeOn(pando::getCurrentPlace(), &matchPurchases, state, lid));
      }
    }
  }
}

// Person < -- SALE -- Person
void matchEEPurchase(State state, Graph::VertexTopologyID lid, Graph::EdgeHandle eh) {
  Graph::VertexTopologyID dst = state.graph.getEdgeDst(eh);
  pando::GlobalRef<bool> pub_ref = state.pub_seller[state.graph.getVertexIndex(lid)];
  if (!pub_ref)
    return;
  wf2::partial::Edge edge = state.graph.getEdgeData(eh);
  swapMaxDate(&(state.trans_date[state.graph.getVertexIndex(dst)]), edge.e.sale.date);
  pando::GlobalRef<bool> ref = state.purchase_ee[state.graph.getVertexIndex(dst)];
  if (ref)
    return;
  ref = true;
  DBG_PRINT_PARTIAL(std::cout << state.graph.getVertexIndex(dst) << " pub_buyer\n";)
  PANDO_CHECK(pando::executeOn(pando::getCurrentPlace(), &matchPurchases, state, dst));
}

// Person < -- WRITTEN_BY -- Publication
void matchPersonPub(State state, Graph::VertexTopologyID lid, Graph::EdgeHandle eh) {
  Graph::VertexTopologyID dst = state.graph.getEdgeDst(eh);
  pando::GlobalRef<bool> pub_ref = state.pub[state.graph.getVertexIndex(lid)];
  if (!pub_ref)
    return;
  pando::GlobalRef<bool> ref = state.pub_seller[state.graph.getVertexIndex(dst)];
  if (ref)
    return;
  ref = true;
  DBG_PRINT_PARTIAL(std::cout << lid << " pub_seller\n";)

  for (Graph::EdgeHandle buyer_eh : state.graph.edges(dst)) {
    wf2::partial::Edge buyer_edge = state.graph.getEdgeData(buyer_eh);
    Graph::VertexTopologyID buyer_dst = state.graph.getEdgeDst(buyer_eh);
    wf2::partial::Vertex buyer_node = state.graph.getData(buyer_dst);

    if (buyer_edge.type == agile::TYPES::SALE && buyer_node.type == agile::TYPES::PERSON &&
        buyer_edge.e.sale.product == 11650) {
      PANDO_CHECK(
          pando::executeOn(pando::getCurrentPlace(), &matchEEPurchase, state, dst, buyer_eh));
    }
  }
}

// Publication
void matchPub(State state, Graph::VertexTopologyID lid) {
  pando::GlobalRef<bool> pub_ref = state.pub[state.graph.getVertexIndex(lid)];
  if (pub_ref)
    return;
  pando::GlobalRef<bool> ref1 = state.pub_ee[state.graph.getVertexIndex(lid)];
  pando::GlobalRef<bool> ref2 = state.pub_nyc[state.graph.getVertexIndex(lid)];

  if (ref1 && ref2) {
    pub_ref = true;
    DBG_PRINT_PARTIAL(std::cout << lid << " pub\n";)
    for (Graph::EdgeHandle eh : state.graph.edges(lid)) {
      wf2::partial::Edge edge = state.graph.getEdgeData(eh);
      Graph::VertexTopologyID dst = state.graph.getEdgeDst(eh);
      wf2::partial::Vertex dst_node = state.graph.getData(dst);

      if (edge.type == agile::TYPES::WRITTENBY && dst_node.type == agile::TYPES::PERSON) {
        PANDO_CHECK(pando::executeOn(pando::getCurrentPlace(), &matchPersonPub, state, lid, eh));
      }
    }
  }
}

// Publication --- Has Topic ---> Topic
void matchPubEE(State& state, Graph::VertexTopologyID& lid, Graph::EdgeHandle&) {
  pando::GlobalRef<bool> ref = state.pub_ee[state.graph.getVertexIndex(lid)];
  ref = true;
  DBG_PRINT_PARTIAL(std::cout << state.graph.getVertexIndex(lid) << " pub_ee\n";)
  PANDO_CHECK(pando::executeOn(pando::getCurrentPlace(), &matchPub, state, lid));
}

// Publication --- Has Org ---> Topic
void matchPubNyc(State& state, Graph::VertexTopologyID& lid, Graph::EdgeHandle& eh) {
  Graph::VertexTopologyID dst = state.graph.getEdgeDst(eh);
  wf2::partial::Vertex dst_node = state.graph.getData(dst);

  pando::GlobalRef<bool> ref = state.pub_nyc[state.graph.getVertexIndex(lid)];
  if (ref)
    return;
  if (proximity(dst_node.v.topic)) {
    ref = true;
    DBG_PRINT_PARTIAL(std::cout << state.graph.getVertexIndex(lid) << " pub_nyc\n";)
    PANDO_CHECK(pando::executeOn(pando::getCurrentPlace(), &matchPub, state, lid));
  }
}

// Expects an edge of the form FORUM -- HASTOPIC -> TOPIC
// Checks if a forum has NYC topic
void matchNyc(State& state, Graph::VertexTopologyID& lid, Graph::EdgeHandle&) {
  pando::GlobalRef<bool> ref = state.nyc[state.graph.getVertexIndex(lid)];
  ref = true;
  DBG_PRINT_PARTIAL(std::cout << lid << " nyc\n";)
}

// Expects an edge of the form FORUMEVENT -- HASTOPIC -> TOPIC
// Checks if a forumevent has Jihad topic
void matchJihad(State& state, Graph::VertexTopologyID& lid, Graph::EdgeHandle&) {
  pando::GlobalRef<std::int64_t> ref = state.jihad[state.graph.getVertexIndex(lid)];
  ref = true;
  DBG_PRINT_PARTIAL(std::cout << lid << " jihad\n";)
}

// Expects an edge of the form FORUMEVENT -- HASTOPIC -> TOPIC
// Checks if a forumevent has 2a topics
void matchFe2A(State& state, Graph::VertexTopologyID& lid, Graph::EdgeHandle& eh) {
  Graph::VertexTopologyID dst = state.graph.getEdgeDst(eh);
  wf2::partial::Vertex node = state.graph.getData(lid);
  wf2::partial::Vertex dst_node = state.graph.getData(dst);

  pando::GlobalRef<bool> ref1 = state.f2a_1[state.graph.getVertexIndex(lid)];
  pando::GlobalRef<bool> ref2 = state.f2a_2[state.graph.getVertexIndex(lid)];
  if (state.graph.getTokenID(dst) == 1049632) {
    ref1 = true;
  } else if (state.graph.getTokenID(dst) == 69871376) {
    ref2 = true;
  }
  if (ref1 && ref2) {
    for (auto dst_eh : state.graph.edges(lid)) {
      wf2::partial::Edge dst_edge = state.graph.getEdgeData(dst_eh);
      Graph::VertexTopologyID forum_dst = state.graph.getEdgeDst(dst_eh);
      wf2::partial::Vertex forum_node = state.graph.getData(forum_dst);
      if (dst_edge.type == agile::TYPES::INCLUDEDIN && forum_node.type == agile::TYPES::FORUM) {
        swapMinDate(&(state.forum_date[state.graph.getVertexIndex(forum_dst)]),
                    node.v.forum_event.date);
        DBG_PRINT_PARTIAL(std::cout << state.forum_date[state.graph.getVertexIndex(forum_dst)]
                                    << " " << node.v.forum_event.date << " date 2a\n";)
        pando::GlobalRef<bool> f2a_ref = state.f2a[state.graph.getVertexIndex(forum_dst)];
        f2a_ref = true;
        PANDO_CHECK(pando::executeOn(pando::getCurrentPlace(), &matchFe2A2B, state, forum_dst));
        break;
      }
    }
  }
}

// ForumEvent --- Has Topic ---> Topic
// Expects an edge of the form FORUMEVENT -- HASTOPIC -> TOPIC
// Checks if a forumevent has 2b topics
void matchFe2B(State& state, Graph::VertexTopologyID& lid, Graph::EdgeHandle& eh) {
  wf2::partial::Edge edge = state.graph.getEdgeData(eh);
  Graph::VertexTopologyID dst = state.graph.getEdgeDst(eh);
  wf2::partial::Vertex node = state.graph.getData(lid);
  wf2::partial::Vertex dst_node = state.graph.getData(dst);

  if (node.type == agile::TYPES::FORUMEVENT && edge.type == agile::TYPES::HASTOPIC &&
      dst_node.type == agile::TYPES::TOPIC) {
    pando::GlobalRef<bool> ref1 = state.f2b_1[state.graph.getVertexIndex(lid)];
    pando::GlobalRef<bool> ref2 = state.f2b_2[state.graph.getVertexIndex(lid)];
    pando::GlobalRef<bool> ref3 = state.f2b_3[state.graph.getVertexIndex(lid)];
    if (state.graph.getTokenID(dst) == 127197) {
      ref1 = true;
    } else if (state.graph.getTokenID(dst) == 179057) {
      ref2 = true;
    } else if (state.graph.getTokenID(dst) == 771572) {
      ref3 = true;
    }
    if (ref1 && ref2 && ref3) {
      for (auto dst_eh : state.graph.edges(lid)) {
        wf2::partial::Edge dst_edge = state.graph.getEdgeData(dst_eh);
        Graph::VertexTopologyID forum_dst = state.graph.getEdgeDst(dst_eh);
        wf2::partial::Vertex forum_node = state.graph.getData(forum_dst);
        if (dst_edge.type == agile::TYPES::INCLUDEDIN && forum_node.type == agile::TYPES::FORUM) {
          swapMinDate(&(state.forum_date[state.graph.getVertexIndex(forum_dst)]),
                      node.v.forum_event.date);
          DBG_PRINT_PARTIAL(std::cout << state.forum_date[state.graph.getVertexIndex(forum_dst)]
                                      << " " << node.v.forum_event.date << " date 2b\n";)
          pando::GlobalRef<bool> f2b_ref = state.f2b[state.graph.getVertexIndex(forum_dst)];
          f2b_ref = true;
          PANDO_CHECK(pando::executeOn(pando::getCurrentPlace(), &matchFe2A2B, state, forum_dst));
          break;
        }
      }
    }
  }
}

// Expects an edge of the form FORUMEVENT -- HASTOPIC -> TOPIC
// Checks if a forumevent has 2 topics
void matchBasicPurchases(State& state, Graph::VertexTopologyID& lid, Graph::EdgeHandle& eh) {
  wf2::partial::Edge edge = state.graph.getEdgeData(eh);

  if (edge.e.sale.product == 2869238 || edge.e.sale.product == 271997) {
    bool old_val = true;
    if (edge.e.sale.product == 2869238) {
      pando::GlobalRef<bool> ref = state.purchase_bb[state.graph.getVertexIndex(lid)];
      old_val = ref;
      ref = true;
    } else if (edge.e.sale.product == 271997) {
      pando::GlobalRef<bool> ref = state.purchase_pc[state.graph.getVertexIndex(lid)];
      old_val = ref;
      ref = true;
    }
    swapMaxDate(&(state.trans_date[state.graph.getVertexIndex(lid)]), edge.e.sale.date);
    if (!old_val) {
      PANDO_CHECK(pando::executeOn(pando::getCurrentPlace(), &matchPurchases, state, lid));
    }
  }
}

// TODO(KavyaVarma) Add conditional checks here
void processEdge(State& state, Graph::VertexTopologyID& lid, Graph::EdgeHandle eh) {
  wf2::partial::Edge edge = state.graph.getEdgeData(eh);
  Graph::VertexTopologyID dst = state.graph.getEdgeDst(eh);
  wf2::partial::Vertex node = state.graph.getData(lid);
  wf2::partial::Vertex dst_node = state.graph.getData(dst);

  if (edge.type == agile::TYPES::PURCHASE && dst_node.type == agile::TYPES::PERSON &&
      edge.e.sale.product == 185785) {
    matchAmmoPurchase(state, lid, eh);
  }
  if (node.type == agile::TYPES::FORUMEVENT && edge.type == agile::TYPES::HASTOPIC &&
      dst_node.type == agile::TYPES::TOPIC) {
    matchFe2A(state, lid, eh);
  }
  if (node.type == agile::TYPES::FORUMEVENT && edge.type == agile::TYPES::HASTOPIC &&
      dst_node.type == agile::TYPES::TOPIC) {
    matchFe2B(state, lid, eh);
  }
  if (node.type == agile::TYPES::FORUM && edge.type == agile::TYPES::HASTOPIC &&
      dst_node.type == agile::TYPES::TOPIC && state.graph.getTokenID(dst) == 60) {
    matchNyc(state, lid, eh);
  }
  if (node.type == agile::TYPES::FORUMEVENT && edge.type == agile::TYPES::HASTOPIC &&
      dst_node.type == agile::TYPES::TOPIC && state.graph.getTokenID(dst) == 44311) {
    matchJihad(state, lid, eh);
  }
  if (node.type == agile::TYPES::PUBLICATION && edge.type == agile::TYPES::HASTOPIC &&
      dst_node.type == agile::TYPES::TOPIC && state.graph.getTokenID(dst) == 43035) {
    matchPubEE(state, lid, eh);
  }
  if (node.type == agile::TYPES::PUBLICATION && edge.type == agile::TYPES::HASORG &&
      dst_node.type == agile::TYPES::TOPIC) {
    matchPubNyc(state, lid, eh);
  }
  if (node.type == agile::TYPES::PERSON && edge.type == agile::TYPES::PURCHASE &&
      dst_node.type == agile::TYPES::PERSON) {
    matchBasicPurchases(state, lid, eh);
  }
  if (node.type == agile::TYPES::PUBLICATION && edge.type == agile::TYPES::WRITTENBY &&
      dst_node.type == agile::TYPES::PERSON) {
    matchPersonPub(state, lid, eh);
  }
}

void processVertex(State state, Graph::VertexTopologyID lid) {
  for (auto eh : state.graph.edges(lid)) {
    processEdge(state, lid, eh);
  }
}

std::uint64_t sum(pando::Vector<bool> arr) {
  std::uint64_t acc = 0;
  for (bool x : arr)
    acc += (x ? 1 : 0);
  return acc;
}

std::uint64_t sum(pando::Vector<std::int64_t> arr) {
  std::uint64_t acc = 0;
  for (std::int64_t x : arr)
    acc += x;
  return acc;
}

void partialMatch(pando::GlobalPtr<Graph> graph_ptr) {
  Graph graph = *(graph_ptr);
  State state(graph);
  state.initialize(graph.size());

  galois::doAll(state, graph.vertices(), &processVertex);
  // galois::doAll(state, state.graph.vertices(), &matchPurchases);
  DBG_PRINT_PARTIAL(std::cout << "Subpattern 12: " << sum(state.sp12) << std::endl;)
  DBG_PRINT_PARTIAL(std::cout << "Number of Jihad Events: " << sum(state.jihad) << std::endl;)
  DBG_PRINT_PARTIAL(std::cout << "Subpattern 1: " << sum(state.f2a_1) << std::endl;)
  DBG_PRINT_PARTIAL(std::cout << "Subpattern 2: " << sum(state.f2a_2) << std::endl;)
  DBG_PRINT_PARTIAL(std::cout << "Subpattern 5: " << sum(state.pub) << std::endl;)
  DBG_PRINT_PARTIAL(std::cout << "Subpattern 6: " << sum(state.interesting_persons) << std::endl;)
  DBG_PRINT_PARTIAL(std::cout << "Subpattern 7: " << sum(state.ammo_dist) << std::endl;)
}

} // namespace partial
} // namespace wf2
