// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF1_LAYERS_TRACE_HPP_
#define PANDO_WF1_LAYERS_TRACE_HPP_

#include <cmath>

#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-rt/containers/array.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-wf1/graphs/gnngraph.hpp>
#include <pando-wf1/layers/layer.hpp>
#include <pando-wf1/math/gnnmath.hpp>

namespace gnn {

/**
 * @brief Graph Linear Network (GCN) layer.
 */
template <typename InnerGraph>
class TraceLayer : public GNNLayer<InnerGraph> {
public:
  using VertexTopologyID = typename InnerGraph::VertexTopologyID;
  using EdgeHandle = typename InnerGraph::EdgeHandle;
  using VertexData = typename InnerGraph::VertexData;
  using EdgeData = typename InnerGraph::EdgeData;
  using VertexRange = typename InnerGraph::VertexRange;
  using VertexIterator = typename InnerGraph::VertexIt;
  using LCSR = typename InnerGraph::CSR;

  constexpr TraceLayer() : GNNLayer<InnerGraph>() {}

  void initialize(std::uint32_t layerNumber,
                  galois::HostLocalStorage<pando::Array<GNNFloat>>& backwardOutputMatrix,
                  galois::HostLocalStorage<GNNLayerDimensions>& dimensions) {
#if DEBUG_PRINTS
    std::cerr << "[Trace Layer " << layerNumber << "] Starts initialization\n" << std::flush;
#endif
    // Call and initialize operand matrices
    GNNLayer<InnerGraph>::initialize(layerNumber, backwardOutputMatrix, dimensions, false);

#ifndef NDEBUG
    for (GNNLayerDimensions dims : dimensions) {
      assert(dims.inputRows == dims.inputCols);
      assert(dims.outputRows == 1 && dims.outputCols == 1);
    }
#endif

#if DEBUG_PRINTS
    std::cerr << "[Trace Layer " << layerNumber << "] Starts initialization [DONE]\n" << std::flush;
#endif
  }

  constexpr TraceLayer(TraceLayer<InnerGraph>&&) noexcept = default;
  constexpr TraceLayer(const TraceLayer<InnerGraph>&) noexcept = default;
  constexpr TraceLayer<InnerGraph>& operator=(const TraceLayer<InnerGraph>&) noexcept = default;
  constexpr TraceLayer<InnerGraph>& operator=(TraceLayer<InnerGraph>&&) noexcept = default;

  /**
   * @brief Start the forward phase of the trace
   */
  template <bool isPull = false>
  const galois::HostLocalStorage<pando::Array<GNNFloat>> ForwardPhase(
      galois::HostLocalStorage<pando::Array<GNNFloat>> inputEmbeddings) {
    auto tpl = galois::make_tpl(inputEmbeddings, this->forwardOutputMatrix_);
    PANDO_CHECK(galois::doAll(
        tpl, this->dimensions_, +[](decltype(tpl) tpl, GNNLayerDimensions dims) {
          auto [inputEmbeds, forwardOutputMat] = tpl;
          pando::Array<GNNFloat> localInp = inputEmbeds.getLocal();
          assert(localInp.size() >= dims.inputRows * dims.inputCols);
          pando::Array<GNNFloat> localOut = forwardOutputMat.getLocal();
          assert(localOut.size() >= 1);
          if constexpr (isPull) {
            GNNFloat f = 0;
            for (std::uint64_t i = 0; i < dims.inputRows; i++) {
              f += localInp[i * dims.inputRows + i];
            }
            localOut[0] = f;
          } else {
            localOut[0] = 0;
            auto innerTpl = galois::make_tpl(localInp, localOut, dims);
            PANDO_CHECK(galois::doAll(
                innerTpl, galois::IotaRange(0, dims.inputRows),
                +[](decltype(innerTpl) tpl, std::uint64_t i) {
                  auto [inpMat, outMat, dims] = tpl;
                  GNNFloat f = inpMat[i * dims.inputRows + i];
                  pando::atomicFetchAdd(&outMat[0], f, std::memory_order_relaxed);
                }));
          }
        }));
    return this->forwardOutputMatrix_;
  }

  /**
   * @brief Start the backward phase of the GCN layer.
   */
  template <bool BackOutIsAlreadyZero = false>
  galois::HostLocalStorage<pando::Array<GNNFloat>> BackwardPhase(
      galois::HostLocalStorage<pando::Array<GNNFloat>>& inputGradients,
      pando::GlobalPtr<graph::GNNGraph<InnerGraph>>, bool) {
    auto tpl = galois::make_tpl(inputGradients, this->backwardOutputMatrix_);
    PANDO_CHECK(galois::doAll(
        tpl, this->dimensions_, +[](decltype(tpl) tpl, GNNLayerDimensions dims) {
          assert(dims.inputRows == dim.inputCols);
          assert(dims.outputCols == 1 && dims.outputRows == 1);
          auto [inputGrads, outputMat] = tpl;
          GNNFloat inputGrad = fmap(inputGrads.getLocal(), operator[], 0);
          pando::Array<GNNFloat> backOut = outputMat.getLocal();
          auto innerTpl = galois::make_tpl(backOut, inputGrad);
          if constexpr (BackOutIsAlreadyZero) {
            PANDO_CHECK(galois::doAll(
                innerTpl, galois::IotaRange(0, dims.inputRows),
                +[](decltype(innerTpl) tpl, std::uint64_t i) {
                  auto [backOut, inputGrad] = tpl;
                  const long double rowsD = std::sqrt(backOut.size());
                  const std::uint64_t rows = static_cast<std::uint64_t>(rowsD);
                  backOut[rows * i + i] = inputGrad;
                }));
          } else {
            PANDO_CHECK(galois::doAll(
                innerTpl, galois::IotaRange(0, dims.inputRows * dims.inputRows),
                +[](decltype(innerTpl) tpl, std::uint64_t i) {
                  auto [backOut, inputGrad] = tpl;
                  const long double rowsD = std::sqrt(backOut.size());
                  const std::uint64_t rows = static_cast<std::uint64_t>(rowsD);
                  std::uint64_t row = i / rows;
                  std::uint64_t col = i % rows;
                  backOut[row * rows + col] = (row == col) ? inputGrad : 0;
                }));
          }
        }));
    return this->backwardOutputMatrix_;
  }
};

} // namespace gnn

#endif // PANDO_WF1_LAYERS_TRACE_HPP_
