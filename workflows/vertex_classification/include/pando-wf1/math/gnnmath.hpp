// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF1_MATH_GNNMATH_HPP_
#define PANDO_WF1_MATH_GNNMATH_HPP_

#include <utility>

#include <pando-lib-galois/containers/per_host.hpp>
#include <pando-lib-galois/sync/atomic.hpp>
#include <pando-lib-galois/utility/tuple.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-wf1/gnntypes.hpp>

namespace gnn {

/**
 * @brief A collection of math kernels.
 */
template <typename VTy, typename ETy>
class GNNMath {
public:
  GNNMath() = default;

private:
};

/**
 * Matrix multiplication perhost
 *
 */
template <bool isPull = false, bool cIsAlreadyZeroInit = false>
pando::Status multiplyMatricesPerHost(galois::PerHost<pando::Array<GNNFloat>> a,
                                      galois::PerHost<pando::Array<GNNFloat>> b,
                                      galois::PerHost<pando::Array<GNNFloat>> c,
                                      galois::PerHost<gnn::GNNLayerDimensions> dims) {
  using galois::make_tpl;
  assert(a != c);
  assert(b != c);
  using AF = pando::Array<GNNFloat>;

  auto nextTpl = make_tpl(a, b, c);
  PANDO_CHECK_RETURN(galois::doAll(
      nextTpl, dims, +[](decltype(nextTpl) tpl, GNNLayerDimensions dim) {
        galois::WaitGroup wg;
        PANDO_CHECK(wg.initialize(0));
        auto wgh = wg.getHandle();
        uint64_t host = pando::getCurrentPlace().node.id;
        auto [a, b, c] = tpl;
        AF localA = fmap(a, get, host);
        AF localB = fmap(b, get, host);
        AF localC = fmap(c, get, host);

        if constexpr (!isPull && !cIsAlreadyZeroInit) {
          PANDO_CHECK(galois::doAll(
              wgh, localC, +[](pando::GlobalRef<GNNFloat> v) {
                v = 0;
              }));
          PANDO_CHECK(wg.wait());
        }
        auto nextTpl = make_tpl(wgh, localA, localB, localC, dim);
        PANDO_CHECK(galois::doAll(
            wgh, nextTpl, galois::IotaRange(0, dim.inputRows),
            +[](decltype(nextTpl) tpl, LayerDimension r) {
              auto [wgh, localA, localB, localC, dim] = tpl;
              if constexpr (isPull) {
                auto nextTpl = make_tpl(localA, localB, localC, dim, r);
                PANDO_CHECK(galois::doAll(
                    wgh, nextTpl, galois::IotaRange(0, dim.outputColumns),
                    +[](decltype(nextTpl) tpl, LayerDimension z) {
                      auto [localA, localB, localC, dim, r] = tpl;
                      GNNFloat accum = 0;
                      for (LayerDimension x : galois::IotaRange(0, dim.inputColumns)) {
                        GNNFloat p = localA[r * dim.inputColumns + x];
                        GNNFloat q = localB[x * dim.outputColumns + z];
                        accum += p * q;
                      }
                      localC[r * dim.outputColumns + z] = accum;
                    }));
              } else {
                auto nextTpl = make_tpl(wgh, localA, localB, localC, dim, r);
                PANDO_CHECK(galois::doAll(
                    wgh, nextTpl, galois::IotaRange(0, dim.outputColumns),
                    +[](decltype(nextTpl) tpl, LayerDimension z) {
                      auto [wgh, localA, localB, localC, dim, r] = tpl;
                      auto nextTpl = make_tpl(localA, localB, localC, dim, r, z);
                      PANDO_CHECK(galois::doAll(
                          wgh, nextTpl, galois::IotaRange(0, dim.inputColumns),
                          +[](decltype(nextTpl) tpl, LayerDimension x) {
                            auto [localA, localB, localC, dim, r, z] = tpl;
                            GNNFloat p = localA[r * dim.inputColumns + x];
                            GNNFloat q = localB[x * dim.outputColumns + z];
                            pando::atomicFetchAdd(&localC[r * dim.outputColumns + z], p * q,
                                                  std::memory_order_relaxed);
                          }));
                    }));
              }
            }));
        PANDO_CHECK(wg.wait());
      }));
  return pando::Status::Success;
}

} // namespace gnn

#endif // PANDO_WF1_MATH_GNNMATH_HPP_
