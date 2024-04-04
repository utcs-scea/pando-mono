// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF1_GALOIS_LAYERS_SOFTMAX_HPP_
#define PANDO_WF1_GALOIS_LAYERS_SOFTMAX_HPP_

#include <limits>

#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-wf1-galois/layers/layer.hpp>

namespace gnn {

/**
 * @brief Softmax layer.
 */
template <typename InnerGraph>
class SoftmaxLayer : public GNNLayer<InnerGraph> {
public:
  using VertexTopologyID = typename InnerGraph::VertexTopologyID;
  using VertexData = typename InnerGraph::VertexData;

  constexpr SoftmaxLayer() : GNNLayer<InnerGraph>() {}
  constexpr SoftmaxLayer(SoftmaxLayer<InnerGraph>&&) noexcept = default;
  constexpr SoftmaxLayer(const SoftmaxLayer<InnerGraph>&) noexcept = default;
  constexpr SoftmaxLayer<InnerGraph>& operator=(const SoftmaxLayer<InnerGraph>&) noexcept = default;
  constexpr SoftmaxLayer<InnerGraph>& operator=(SoftmaxLayer<InnerGraph>&&) noexcept = default;

  void initialize(std::uint32_t layerNumber,
                  galois::PerHost<pando::Array<GNNFloat>>& backwardOutputMatrix,
                  galois::PerHost<GNNLayerDimensions> dimensions) {
    std::cout << "[Softmax] Starts initialization\n" << std::flush;
    GNNLayer<InnerGraph>::initialize(layerNumber, backwardOutputMatrix, dimensions, false);
    std::cout << "[Softmax] Starts initialization [DONE]\n" << std::flush;
  }

  /**
   * @brief Start the forward phase of the softmax layer.
   */
  const galois::PerHost<pando::Array<GNNFloat>> ForwardPhase(
      galois::PerHost<pando::Array<GNNFloat>> inputEmbeddings,
      pando::GlobalPtr<graph::GNNGraph<InnerGraph>> gPtr) {
    struct Tpl {
      galois::PerHost<GNNLayerDimensions> dim;
      galois::PerHost<pando::Array<GNNFloat>> outMat;
      galois::PerHost<pando::Array<GNNFloat>> inMat;
      std::uint64_t numClasses;
    };

    struct InnerTpl {
      pando::Array<GNNFloat> inMat;
      pando::Array<GNNFloat> outMat;
      std::uint64_t numClasses;
    };

#if 0
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      std::cout << "[[ SOFTMAX PRE FORWARD ]]\n";
      pando::GlobalRef<pando::Array<GNNFloat>> outRef =
          fmap(this->backwardOutputMatrix_, get, host);
      pando::Array<GNNFloat> out = outRef;
      pando::Array<GNNFloat> in = fmap(inputEmbeddings, get, host);
      for (std::uint64_t i = 0; i < lift(outRef, size); ++i) {
        std::cout << i << ": in:" << in[i] << " out:" << out[i] << "\n";
      }
    }
#endif

    galois::doAll(
        Tpl{this->dimensions_, this->backwardOutputMatrix_, inputEmbeddings,
            lift(*gPtr, GetNumClasses)},
        this->backwardOutputMatrix_, +[](Tpl tpl, pando::Array<GNNFloat> outMat) {
          std::uint32_t host = pando::getCurrentPlace().node.id;
          pando::Array<GNNFloat> inMat = fmap(tpl.inMat, get, host);
          GNNLayerDimensions dim = fmap(tpl.dim, get, host);

          galois::doAll(
              InnerTpl{inMat, outMat, tpl.numClasses}, galois::IotaRange(0, dim.inputRows),
              +[](InnerTpl tpl, LayerDimension i) {
                // Get an inferred vertex class
                std::uint64_t numClasses = tpl.numClasses;
                GNNFloat maxElem{-std::numeric_limits<GNNFloat>::max()};
                for (std::uint64_t f = 0; f < numClasses; ++f) {
                  if (tpl.inMat[i * numClasses + f] > maxElem) {
                    maxElem = tpl.inMat[i * numClasses + f];
                  }
                }

                GNNFloat denom{0};
                for (std::uint64_t f = 0; f < numClasses; ++f) {
                  tpl.outMat[i * numClasses + f] =
                      std::exp(tpl.inMat[i * numClasses + f] - maxElem);
                  denom += tpl.outMat[i * numClasses + f];
                }

                if (denom > 0) {
                  for (std::uint64_t f = 0; f < numClasses; ++f) {
                    tpl.outMat[i * numClasses + f] /= denom;
                  }
                }
              });
        });

#if 0
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      std::cout << "[[ SOFTMAX FORWARD ]]\n";
      pando::GlobalRef<pando::Array<GNNFloat>> outRef =
          fmap(this->backwardOutputMatrix_, get, host);
      pando::Array<GNNFloat> out = outRef;
      for (std::uint64_t i = 0; i < lift(outRef, size); ++i) {
        std::cout << i << ":" << out[i] << "\n";
      }
    }
#endif

    return this->backwardOutputMatrix_;
  }

  /**
   * @brief Start the backward phase that follows
   * https://gombru.github.io/2018/05/23/cross_entropy_loss/.
   * Derivation of the full combined derivative is not there,
   * but some empirical inspection showed that this is likely correct.
   */
  galois::PerHost<pando::Array<GNNFloat>> BackwardPhase(
      pando::GlobalPtr<graph::GNNGraph<InnerGraph>> gPtr) {
    struct InnerTpl {
      graph::GNNGraph<InnerGraph> g;
      pando::Array<GNNFloat> outMat;
    };

    galois::doAll(
        static_cast<graph::GNNGraph<InnerGraph>>(*gPtr), this->backwardOutputMatrix_,
        +[](graph::GNNGraph<InnerGraph> g, pando::Array<GNNFloat> outMat) {
          std::uint32_t host = pando::getCurrentPlace().node.id;

          VertexDenseID subgraphSize = fmap(g, GetSubgraphSize, host);

          galois::doAll(
              InnerTpl{g, outMat}, galois::IotaRange(0, subgraphSize),
              +[](InnerTpl tpl, VertexDenseID subVid) {
                std::uint32_t host = pando::getCurrentPlace().node.id;

                std::uint64_t numClasses = tpl.g.GetNumClasses();
                pando::Array<GNNFloat> outMat = tpl.outMat;

                // Get ground truth
                VertexDenseID vid = tpl.g.GetVIdFromSubgraphVId(host, subVid);
                VertexTopologyID v = tpl.g.GetTopologyIDFromIndex(vid);
                VertexData vData = tpl.g.GetData(v);
                VertexDenseID groundTruth = tpl.g.GetGroundTruth(vData.type);
                for (LayerDimension c = 0; c < numClasses; ++c) {
                  if (c == groundTruth) {
                    outMat[subVid * numClasses + c] -= 1;
                  }
                }
              });
        });
    return this->backwardOutputMatrix_;
  }
};

} // namespace gnn

#endif // PANDO_WF1_GALOIS_LAYERS_SOFTMAX_HPP_
