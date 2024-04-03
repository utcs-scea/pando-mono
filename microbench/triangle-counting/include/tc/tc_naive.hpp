// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GAL_MICROBENCH_TRIANGLE_COUNTING_INCLUDE_TC_NAIVE_HPP_
#define PANDO_LIB_GAL_MICROBENCH_TRIANGLE_COUNTING_INCLUDE_TC_NAIVE_HPP_

#include <pando-rt/export.h>

#include <cmath>
#include <utility>

#include "tc/utils.hpp"

#include <pando-lib-galois/graphs/graph_traits.hpp>
#include <pando-lib-galois/utility/search.hpp>

namespace galois {

constexpr std::uint64_t upperTriangleToLinear(std::uint64_t i, std::uint64_t j, std::uint64_t n) {
  return n * i + j - ((i * (i + 1)) >> 1);
}
constexpr std::pair<std::uint64_t, std::uint64_t> linearToUpperTriangle(std::uint64_t k,
                                                                        std::uint64_t n) {
  std::uint64_t root = sqrt((2 * n + 1) * (2 * n + 1) - 8 * (k + 1));
  std::uint64_t i = n - 1 - floor((-1 + root) / 2);
  std::uint64_t j = k - (i * (2 * n - i - 1) >> 1);
  return std::make_pair(i, j);
}

template <typename G>
struct TCStepZero {
  pando::GlobalPtr<G> g;
  galois::DAccumulator<std::uint64_t> sum;
  galois::WaitGroup::HandleType wgh;
};

/**
 * @brief Checks to see if the particular edge out of B goes to C.
 * @warning This function is very sensitive and requires a special atomicAdd to stop deadlock
 */
template <typename G>
void triangleCountInnerLoop(TCStepZero<G> state, typename graph_traits<G>::VertexTopologyID a,
                            typename graph_traits<G>::EdgeRange::iterator wbegin,
                            typename graph_traits<G>::EdgeRange::iterator wend,
                            typename graph_traits<G>::VertexTopologyID propB) {
  if (propB <= a) {
    state.wgh.done();
    return;
  }

  G graph = *(state.g);
  auto lub = graph.edges(propB).begin().m_iter;
  auto bEnd = graph.edges(propB).end().m_iter;

  for (auto currPropC = wbegin;
       currPropC != wend && ((lub = galois::lower_bound(lub, bEnd, *(currPropC.m_iter))) != bEnd);
       currPropC++) {
    typename graph_traits<G>::VertexTopologyID propC = *(currPropC.m_iter);

    if (propC <= a)
      continue;

    if (*lub == propC) {
      state.sum.increment();
    }
  }
  state.wgh.done();
}

/**
 * @brief Takes in a candidate for a then determines if it can be used to generate a triangle
 */
template <typename G>
void triangleCountStepZero(TCStepZero<G> state, typename graph_traits<G>::VertexTopologyID a) {
  G graph = *(state.g);
  std::uint64_t numEdges = graph.getNumEdges(a);
  if (numEdges < 2) {
    return;
  }
  for (auto currB = graph.edges(a).begin(); currB != graph.edges(a).end(); currB++) {
    typename graph_traits<G>::VertexTopologyID propB = *(currB.m_iter);
    state.wgh.add(1);
    PANDO_CHECK(pando::executeOn(graph.getLocalityVertex(propB), &triangleCountInnerLoop<G>, state,
                                 a, currB + 1, graph.edges(a).end(), propB));
  }
}

/**
 * @brief A TC created for direction optimized graph inputs.
 * That takes in a Graph of type G.
 */
template <typename G>
pando::Status DirOptNaiveTC(pando::GlobalPtr<G> g_ptr, pando::GlobalPtr<std::uint64_t> answer_ptr) {
  static_assert(std::is_same<std::uint64_t, typename graph_traits<G>::VertexTopologyID>::value);
  pando::Status err;
  G graph = *g_ptr;

  galois::WaitGroup wg;
  err = wg.initialize(0);
  if (err != pando::Status::Success) {
    return err;
  }

  TCStepZero<G> initialStep{};
  initialStep.g = g_ptr;
  PANDO_CHECK(initialStep.sum.initialize());
  initialStep.wgh = wg.getHandle();

  PANDO_CHECK(
      galois::doAll(wg.getHandle(), initialStep, graph.vertices(), &triangleCountStepZero<G>));
  PANDO_CHECK(wg.wait());
  *answer_ptr = initialStep.sum.reduce();
  initialStep.sum.deinitialize();
  wg.deinitialize();
  return err;
}

} // namespace galois
#endif // PANDO_LIB_GAL_MICROBENCH_TRIANGLE_COUNTING_INCLUDE_TC_NAIVE_HPP_
