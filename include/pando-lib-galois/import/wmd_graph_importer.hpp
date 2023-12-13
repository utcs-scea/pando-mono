// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_IMPORT_WMD_GRAPH_IMPORTER_HPP_
#define PANDO_LIB_GALOIS_IMPORT_WMD_GRAPH_IMPORTER_HPP_

#include <pando-rt/export.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <limits>
#include <string>
#include <utility>

#include <pando-lib-galois/containers/hashtable.hpp>
#include <pando-lib-galois/containers/per_host.hpp>
#include <pando-lib-galois/containers/per_thread.hpp>
#include <pando-lib-galois/graphs/wmd_graph.hpp>
#include <pando-lib-galois/import/ifstream.hpp>
#include <pando-lib-galois/import/schema.hpp>
#include <pando-lib-galois/utility/dist_accumulator.hpp>
#include <pando-lib-galois/utility/gptr_monad.hpp>
#include <pando-lib-galois/utility/pair.hpp>
#include <pando-lib-galois/utility/string_view.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/notification.hpp>

namespace galois::internal {

/**
 * @brief This is used to parallel insert Edges
 */
template <typename EdgeType>
[[nodiscard]] inline pando::Status insertLocalEdgesPerThread(
    pando::GlobalRef<galois::HashTable<std::uint64_t, std::uint64_t>> hashRef,
    galois::PerThreadVector<pando::Vector<EdgeType>> localEdges, EdgeType edge) {
  uint64_t result;

  if (fmap(hashRef, get, edge.src, result)) { // if token already exists
    pando::GlobalRef<pando::Vector<EdgeType>> vec = fmap(localEdges.getThreadVector(), get, result);
    return fmap(vec, pushBack, edge);
  } else { // not exist, make a new one
    fmap(hashRef, put, edge.src, lift(localEdges.getThreadVector(), size));
    pando::Vector<EdgeType> v;
    auto err = v.initialize(1);
    if (err != pando::Status::Success) {
      return err;
    }
    v[0] = std::move(edge);
    return localEdges.pushBack(std::move(v));
  }
}

} // namespace galois::internal

#endif // PANDO_LIB_GALOIS_IMPORT_WMD_GRAPH_IMPORTER_HPP_
