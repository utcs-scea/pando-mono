// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF1_LAYERS_FCN_HPP_
#define PANDO_WF1_LAYERS_FCN_HPP_

#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/utility/tuple.hpp>

#include <pando-rt/containers/array.hpp>
#include <pando-rt/containers/vector.hpp>

#include <pando-wf1/graphs/gnngraph.hpp>
#include <pando-wf1/layers/layer.hpp>
#include <pando-wf1/math/gnnmath.hpp>

namespace gnn {

/**
 * @brief Fully-connected Network (FCN) layer.
 */
template <typename InnerGraph>
class FullyConnectedLayer : public GNNLayer<InnerGraph> {
public:
  constexpr FullyConnectedLayer() : GNNLayer<InnerGraph>() {}

  void initialize(std::uint32_t layerNumber,
                  galois::HostLocalStorage<pando::Array<GNNFloat>>& backwardOutputMatrix,
                  galois::HostLocalStorage<GNNLayerDimensions>& dimensions, bool useDropout,
                  bool useReLU) {
    this->useDropout_ = useDropout;
    this->useReLU_ = useReLU;

#ifdef DEBUG_PRINTS
    std::cerr << "[FCN Layer " << layerNumber << "] Starts initialization\n" << std::flush;
    std::cerr << "[FCN Layer " << layerNumber << "] Dropout setting:" << this->useDropout_ << "\n"
              << std::flush;
    std::cerr << "[FCN Layer " << layerNumber << "] Activation setting:" << this->useReLU_ << "\n"
              << std::flush;
#endif // DEBUG_PRINTS

    // Call and initialize operand matrices
    GNNLayer<InnerGraph>::initialize(layerNumber, backwardOutputMatrix, dimensions, true);
    this->InitializeTempMatrices();
    std::cout << "[FCN Layer " << layerNumber << "] Starts initialization [DONE]\n" << std::flush;
  }

  constexpr FullyConnectedLayer(FullyConnectedLayer<InnerGraph>&&) noexcept = default;
  constexpr FullyConnectedLayer(const FullyConnectedLayer<InnerGraph>&) noexcept = default;
  constexpr FullyConnectedLayer<InnerGraph>& operator=(
      const FullyConnectedLayer<InnerGraph>&) noexcept = default;
  constexpr FullyConnectedLayer<InnerGraph>& operator=(FullyConnectedLayer<InnerGraph>&&) noexcept =
      default;

  /**
   * @brief Allocates operand matrices.
   */
  void InitializeTempMatrices() {
    using galois::make_tpl;

    PANDO_CHECK(this->tempInputMatrix1_.initialize());
    PANDO_CHECK(this->tempOutputMatrix_.initialize());

    auto inTpl = make_tpl(this->tempInputMatrix1_, this->tempOutputMatrix_);

    PANDO_CHECK_RETURN(galois::doAll(
        inTpl, +[](decltype(inTpl) tpl, GNNLayerDimensions dimension) {
          std::uint32_t host = pando::getCurrentPlace().node.id;

          // std::cout << "Dimension:" << dimension.inputRows << "x" << dimension.inputColumns;
          // std::cout << ", " << dimension.outputRows << "x" << dimension.outputColumns << "\n";

          LayerDimension inDim = dimension.inputRows * dimension.inputColumns;
          LayerDimension gradDim = dimension.inputRows * dimension.outputColumns;

          auto [perHostTmpin1, perHostTmpin2] = tpl;

          pando::GlobalRef<pando::Array<GNNFloat>> tmpin1 = *fmap(perHostTmpin1, get, host);
          pando::GlobalRef<pando::Array<GNNFloat>> tmpout = *fmap(perHostTmpin2, get, host);

          PANDO_CHECK(fmap(tmpin1, initialize, inDim));
          PANDO_CHECK(fmap(tmpout, initialize, gradDim));
        }));
  }

  /**
   * @brief Start the forward phase of the FCN layer.
   */
  const galois::HostLocalStorage<pando::Array<GNNFloat>> ForwardPhase(
      galois::HostLocalStorage<pando::Array<GNNFloat>>& inputEmbeddings) {
    if (this->useDropout_) {
      this->DoDropout(inputEmbeddings, this->tempInputMatrix1_);
    }
    this->UpdateEmbedding(this->tempInputMatrix1_, this->forwardOutputMatrix_);
    if (this->useReLU_) {
      this->ReLUActivation();
    }
    return this->forwardOutputMatrix_;
  }

  /**
   * @brief Start the backward phase of the FCN layer.
   */
  galois::HostLocalStorage<pando::Array<GNNFloat>> BackwardPhase(
      galois::HostLocalStorage<pando::Array<GNNFloat>>& inputGradients) {
    if (this->useReLU_) {
      this->ReLUActivationDerivative(inputGradients);
    }

    // W` = F^T * input gradient
    this->CalculateWeightGradient(this->tempInputMatrix1_, inputGradients,
                                  this->layerWeightGradients_);
    if (this->layerNumber_ != 0) {
      // Layer' = input gradient * W^T
      this->CalculateLayerGradient(inputGradients, this->tempInputMatrix1_);
      this->DoDropoutDerivative();
    }

    return this->backwardOutputMatrix_;
  }

  /**
   * @brief Update vertex embeddings by multiplying the current vertex embeddings times
   * this layer's weight matrix.
   */
  void UpdateEmbedding(galois::HostLocalStorage<pando::Array<GNNFloat>>& inputEmbeddings,
                       galois::HostLocalStorage<pando::Array<GNNFloat>>& outputMatrix) {
    gnn::multiplyMatricesPerHost(inputEmbeddings, this->layerWeights_, outputMatrix,
                                 this->dimensions_);
  }

  /**
   * @brief This calculates weight gradient.
   * So, transposed input embeddings (input column x input row) x gradient (input row x output
   * column)
   */
  void CalculateWeightGradient(galois::HostLocalStorage<pando::Array<GNNFloat>>& inputEmbeddings,
                               galois::HostLocalStorage<pando::Array<GNNFloat>>& inputGradients,
                               galois::HostLocalStorage<pando::Array<GNNFloat>>& outputMatrix) {
    using galois::make_tpl;

    auto outTpl = make_tpl(this->dimensions_, inputEmbeddings, inputGradients);

    galois::doAll(
        outTpl, outputMatrix, +[](decltype(outTpl) tpl, pando::Array<GNNFloat> outMat) {
          std::uint32_t host = pando::getCurrentPlace().node.id;

          auto [perHostDim, perHostInMat, perHostInGradMat] = tpl;

          GNNLayerDimensions dim = *fmap(perHostDim, get, host);
          pando::Array<GNNFloat> inMat = *fmap(perHostInMat, get, host);
          pando::Array<GNNFloat> inGradMat = *fmap(perHostInGradMat, get, host);

          // Reset an out matrix
          galois::doAll(
              outMat, +[](pando::GlobalRef<GNNFloat> v) {
                v = 0;
              });

          auto inTpl = make_tpl(dim, outMat, inMat, inGradMat);

          galois::doAll(
              inTpl, galois::IotaRange(0, dim.inputColumns),
              +[](decltype(inTpl) tpl, LayerDimension c) {
                auto [dim, outMat, inMat, inGradMat] = tpl;
                for (LayerDimension z = 0; z < dim.outputColumns; ++z) {
                  for (LayerDimension x = 0; x < dim.outputRows; ++x) {
                    outMat[c * dim.outputColumns + z] +=
                        inMat[x * dim.inputColumns + c] * inGradMat[x * dim.outputColumns + z];
                  }
                }
              });
        });
  }

  /**
   * @brief This calculates layer embedding gradient.
   * So, gradient (input row x output column) x transposed weight (output column x input columns)
   */
  void CalculateLayerGradient(galois::HostLocalStorage<pando::Array<GNNFloat>>& inputGradients,
                              galois::HostLocalStorage<pando::Array<GNNFloat>>& outputMatrix) {
    using galois::make_tpl;

    auto outTpl = make_tpl(this->dimensions_, inputGradients, this->layerWeights_);

    struct InnerTpl {
      GNNLayerDimensions dim;
      pando::Array<GNNFloat> outMat;
      pando::Array<GNNFloat> inGradMat;
      pando::Array<GNNFloat> weightMat;
    };

    galois::doAll(
        outTpl, outputMatrix, +[](decltype(outTpl) tpl, pando::Array<GNNFloat> outMat) {
          std::uint32_t host = pando::getCurrentPlace().node.id;

          galois::doAll(
              outMat, +[](pando::GlobalRef<GNNFloat> v) {
                v = 0;
              });

          auto [perHostDim, perHostInGradMat, perHostWeightMat] = tpl;
          GNNLayerDimensions dim = *fmap(perHostDim, get, host);
          pando::Array<GNNFloat> inGradMat = *fmap(perHostInGradMat, get, host);
          pando::Array<GNNFloat> weightMat = *fmap(perHostWeightMat, get, host);

          auto inTpl = make_tpl(dim, outMat, inGradMat, weightMat);

          galois::doAll(
              inTpl, galois::IotaRange(0, dim.inputRows),
              +[](decltype(inTpl) tpl, LayerDimension r) {
                auto [dim, outMat, inGradMat, weightMat] = tpl;
                for (LayerDimension z = 0; z < dim.inputColumns; ++z) {
                  for (LayerDimension x = 0; x < dim.outputColumns; ++x) {
                    outMat[r * dim.inputColumns + z] +=
                        inGradMat[r * dim.outputColumns + x] * weightMat[z * dim.outputColumns + x];
                  }
                }
              });
        });
  }

private:
  galois::HostLocalStorage<pando::Array<GNNFloat>> tempInputMatrix1_;
  galois::HostLocalStorage<pando::Array<GNNFloat>> tempOutputMatrix_;
  bool useDropout_;
  bool useReLU_;
};

} // namespace gnn

#endif // PANDO_WF1_LAYERS_FCN_HPP_
