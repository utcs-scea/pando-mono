// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF1_GALOIS_LAYERS_LAYER_HPP_
#define PANDO_WF1_GALOIS_LAYERS_LAYER_HPP_

#include <math.h>
#include <random>

#include <pando-rt/containers/array.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/global_ptr.hpp>

#include <pando-lib-galois/containers/per_host.hpp>
#include <pando-lib-galois/loops/do_all.hpp>

#include <pando-wf1-galois/gnntypes.hpp>
#include <pando-wf1-galois/optimizer.hpp>

namespace gnn {

/**
 * @brief Base neural network layer class.
 *
 * @details It allocates and initializes operand matrices, and defines
 * common layer operations such as dropout or activation.
 */
template <typename Graph>
class GNNLayer {
public:
  constexpr GNNLayer() noexcept = default;
  ~GNNLayer() noexcept = default;
  constexpr GNNLayer(GNNLayer<Graph>&&) noexcept = default;
  constexpr GNNLayer(const GNNLayer<Graph>&) noexcept = default;
  constexpr GNNLayer<Graph>& operator=(const GNNLayer<Graph>&) noexcept = default;
  constexpr GNNLayer<Graph>& operator=(GNNLayer<Graph>&&) noexcept = default;

  void initialize(std::uint32_t layerNumber,
                  galois::PerHost<pando::Array<GNNFloat>>& backwardOutputMatrix,
                  galois::PerHost<GNNLayerDimensions>& dimensions, bool needWeight) {
    this->layerNumber_ = layerNumber;
    this->backwardOutputMatrix_ = backwardOutputMatrix;
    this->dimensions_ = dimensions;
    this->needWeight_ = needWeight;
    this->InitializeMatrices();
  }

  /**
   * @brief Allocate and initialize operand matrices for epochs.
   *
   * @note This method should be called once.
   */
  void InitializeMatrices() {
#ifdef PRINTS
    std::cerr << "[GNNLayer] Initializes matrices\n" << std::flush;
#endif // PRINTS

    // Initialize per-host objects
    PANDO_CHECK(this->forwardOutputMatrix_.initialize());
    PANDO_CHECK(this->reluActivation_.initialize());
    if (this->needWeight_) {
      PANDO_CHECK(this->dropoutMask_.initialize());
      PANDO_CHECK(this->layerWeights_.initialize());
      PANDO_CHECK(this->layerWeightGradients_.initialize());
    }

    struct Tpl {
      galois::PerHost<pando::Array<GNNFloat>> fwMat;
      galois::PerHost<pando::Array<bool>> reluMat;
      galois::PerHost<pando::Array<bool>> doMat;
      galois::PerHost<pando::Array<GNNFloat>> lwMat;
      galois::PerHost<pando::Array<GNNFloat>> lwgMat;
      bool needWeight;
    };

    // Initialize per-host matrices
    galois::doAll(
        Tpl{this->forwardOutputMatrix_, this->reluActivation_, this->dropoutMask_,
            this->layerWeights_, this->layerWeightGradients_, this->needWeight_},
        this->dimensions_, +[](Tpl tpl, GNNLayerDimensions dimension) {
          // Get local matrix dimensions
          LayerDimension inputDim = dimension.inputRows * dimension.inputColumns;
          LayerDimension outputDim = dimension.outputRows * dimension.outputColumns;
          LayerDimension weightDim = dimension.inputColumns * dimension.outputColumns;

          std::uint32_t host = pando::getCurrentPlace().node.id;

          // Get local matrices
          pando::GlobalRef<pando::Array<GNNFloat>> fwMat = fmap(tpl.fwMat, get, host);
          pando::GlobalRef<pando::Array<bool>> reluMat = fmap(tpl.reluMat, get, host);
          pando::GlobalRef<pando::Array<bool>> doMat = fmap(tpl.doMat, get, host);
          pando::GlobalRef<pando::Array<GNNFloat>> lwMat = fmap(tpl.lwMat, get, host);
          pando::GlobalRef<pando::Array<GNNFloat>> lwgMat = fmap(tpl.lwgMat, get, host);

          PANDO_CHECK(fmap(fwMat, initialize, outputDim));
          PANDO_CHECK(fmap(reluMat, initialize, outputDim));

          if (tpl.needWeight) { /* Layer requiring weight: gcn */
            PANDO_CHECK(fmap(doMat, initialize, inputDim));
            PANDO_CHECK(fmap(lwMat, initialize, weightDim));
            PANDO_CHECK(fmap(lwgMat, initialize, weightDim));
          }
        });

    // Initialize an weight matrix through the Glorot-Bengio method
    if (this->needWeight_) {
      this->GlorotBengioWeightInit();
    }
#ifdef PRINTS
    std::cerr << "[GNNLayer] Initializes matrices [DONE]\n" << std::flush;
#endif
  }

  /**
   * @brief Initialize a weight matrix by the Glorot-Bengio method.
   */
  void GlorotBengioWeightInit() {
    std::cout << "[GNNLayer] Initializes weight matrix by Glorot-Bengio\n" << std::flush;
    galois::doAll(
        this->layerNumber_, this->layerWeights_,
        +[](std::uint32_t layerNum, pando::Array<GNNFloat> lw) {
          GNNFloat maxVal = std::sqrt(6.0) / std::sqrt(lift(lw, size));
          std::default_random_engine rng(1 + layerNum);
          std::uniform_real_distribution<GNNFloat> dist(-maxVal, maxVal);

          for (LayerDimension i = 0; i < lw.size(); ++i) {
            lw[i] = dist(rng);
          }
        });

#if 0
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      pando::GlobalRef<pando::Array<GNNFloat>> weightRef = fmap(this->layerWeights_, get, host);
      pando::Array<GNNFloat> wt = weightRef;
      std::cout << host << "'s size:" << wt.size() << "\n";
      for (std::uint64_t i = 0; i < wt.size(); ++i) {
        std::cout << "host:" << host << ", i:" << i << ", weight:" << wt[i] << "\n";
      }
    }
#endif

    std::cout << "[GNNLayer] Initializes weight matrix by Glorot-Bengio [DONE]\n" << std::flush;
  }

  /**
   * @brief Perform dropout over an embedding in which drops some features by setting 0 while
   * rescale other features.
   */
  void DoDropout(galois::PerHost<pando::Array<GNNFloat>>& inputToDropout,
                 galois::PerHost<pando::Array<GNNFloat>>& outputMatrix) {
#if 0
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      GNNLayerDimensions dimensions = fmap(this->dimensions_, get, host);
      pando::GlobalRef<pando::Array<GNNFloat>> inRef = fmap(inputToDropout, get, host);
      pando::GlobalRef<pando::Array<GNNFloat>> outRef = fmap(outputMatrix, get, host);
      PANDO_CHECK(fmap(inRef, initialize, dimensions.inputColumns * dimensions.inputRows));
      PANDO_CHECK(fmap(outRef, initialize, dimensions.inputColumns * dimensions.inputRows));
      pando::Array<GNNFloat> in = inRef;
      pando::Array<GNNFloat> out = outRef;

      for (std::uint64_t i = 0; i < lift(inRef, size); ++i) {
        in[i] = i;
        out[i] = 0;
      }
    }
#endif

    GNNFloat dropoutRate{0.5};
    GNNFloat scale{1.f / (1.f - dropoutRate)};

    struct Tpl {
      RandomNumberGenerator dropoutSampler;
      GNNFloat dropoutRate;
    };
    // Sample drop-out masks based on the Bernoulli method
    galois::doAll(
        Tpl{this->dropoutSampler_, dropoutRate}, this->dropoutMask_,
        +[](Tpl tpl, pando::Array<bool> mask) {
          galois::doAll(
              tpl, mask, +[](Tpl tpl, pando::GlobalRef<bool> v) {
                RandomNumberGenerator dropoutSampler = tpl.dropoutSampler;
                GNNFloat dropoutRate = tpl.dropoutRate;
                v = dropoutSampler.DoBernoulli(dropoutRate);
              });
        });

    struct OutTpl {
      galois::PerHost<GNNLayerDimensions> dimensions;
      galois::PerHost<pando::Array<GNNFloat>> outEmbed;
      galois::PerHost<pando::Array<GNNFloat>> inEmbed;
      GNNFloat scale;
    };

    struct InnerTpl {
      pando::Array<GNNFloat> outEmbed;
      pando::Array<GNNFloat> inEmbed;
      pando::Array<bool> mask;
      GNNFloat scale;
    };

    galois::doAll(
        OutTpl{this->dimensions_, outputMatrix, inputToDropout, scale}, this->dropoutMask_,
        +[](OutTpl tpl, pando::Array<bool> mask) {
          std::uint32_t host = pando::getCurrentPlace().node.id;

          GNNLayerDimensions dimension = fmap(tpl.dimensions, get, host);
          pando::Array<GNNFloat> outEmbed = fmap(tpl.outEmbed, get, host);
          pando::Array<GNNFloat> inEmbed = fmap(tpl.inEmbed, get, host);
          LayerDimension indexRange = dimension.inputColumns * dimension.inputRows;

          galois::doAll(
              InnerTpl{outEmbed, inEmbed, mask, tpl.scale}, galois::IotaRange(0, indexRange),
              +[](InnerTpl tpl, LayerDimension i) {
                pando::Array<GNNFloat> outEmbed = tpl.outEmbed;
                pando::Array<GNNFloat> inEmbed = tpl.inEmbed;
                pando::Array<bool> mask = tpl.mask;
                GNNFloat scale = tpl.scale;
                if (mask[i] == 1) {
                  outEmbed[i] = inEmbed[i] * scale;
                } else {
                  outEmbed[i] = 0;
                }
              });
        });

#if 0
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      pando::Array<GNNFloat> in = fmap(inputToDropout, get, host);
      pando::Array<GNNFloat> out = fmap(outputMatrix, get, host);
      pando::Array<bool> mask = fmap(this->dropoutMask_, get, host);
      std::cout << "[DROPOUT]*********\n";
      for (std::uint64_t i = 0; i < in.size(); ++i) {
        std::cout << "host:" << host << ", i:" << i << ", in:" << in[i] << ", out:" << out[i]
                  << ", mask:" << mask[i] << "\n";
      }
    }

    std::cout << "[GNNLayer] Perform drop-out over vertex embedding [DONE]\n" << std::flush;
#endif
  }

  /**
   * @brief Perform ReLU activation in which a value that is <= 0 is set to 0.
   * This method logs indices of the inactivated feature to ignore them during gradient
   * descent.
   */
  void ReLUActivation() {
    struct InnerTpl {
      pando::Array<GNNFloat> fwOut;
      pando::Array<bool> reluAct;
    };

#if 0
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      pando::Array<GNNFloat> in = fmap(forwardOutputMatrix_, get, host);
      for (std::uint64_t i = 0; i < in.size(); ++i) {
        std::cout << "host:" << host << ", i:" << i << ", in:" << in[i] << "\n";
      }
    }
#endif

    galois::doAll(
        this->forwardOutputMatrix_, this->reluActivation_,
        +[](galois::PerHost<pando::Array<GNNFloat>> fwOuts, pando::Array<bool> reluAct) {
          std::uint32_t host = pando::getCurrentPlace().node.id;

          pando::Array<GNNFloat> fwOut = fmap(fwOuts, get, host);

          // Reset a ReLU activation matrix
          galois::doAll(
              reluAct, +[](pando::GlobalRef<bool> v) {
                v = false;
              });

          galois::doAll(
              InnerTpl{fwOut, reluAct}, galois::IotaRange(0, fwOut.size()),
              +[](InnerTpl tpl, LayerDimension i) {
                pando::GlobalRef<GNNFloat> v = tpl.fwOut[i];
                pando::GlobalRef<bool> activated = tpl.reluAct[i];
                if (v > GNNFloat{0}) {
                  activated = true;
                } else {
                  v = 0;
                }
              });
        });

#if 0
    std::cout << "************ RELUACT FORWARD\n";
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      pando::Array<GNNFloat> in = fmap(this->forwardOutputMatrix_, get, host);
      pando::Array<bool> mask = fmap(this->reluActivation_, get, host);
      for (std::uint64_t i = 0; i < in.size(); ++i) {
        std::cout << "host:" << host << ", i:" << i << ", out:" << in[i] << ", mask:" << mask[i]
                  << "\n";
      }
    }
#endif
  }

  /**
   * @brief Perform leaky ReLU activation in which a value that is <= 0 is set to value * 0.01.
   * Different from ReLU, this does not ignore inactivated features.
   */
  void LeakyReLUActivation() {
    galois::doAll(
        this->forwardOutputMatrix_, +[](pando::Array<GNNFloat> fwOut) {
          galois::doAll(
              fwOut, +[](pando::GlobalRef<GNNFloat> v) {
                if (v < GNNFloat{0}) {
                  v = 0.01 * v;
                }
              });
        });
  }

  /**
   * @brief Get the current layer's forward output matrix.
   */
  galois::PerHost<pando::Array<GNNFloat>> GetForwardOutputMatrix() {
    return this->forwardOutputMatrix_;
  }

  /**
   * @brief Get the current layer's number.
   */
  std::uint32_t GetLayerNumber() {
    return this->layerNumber_;
  }

  /**
   * @brief Inactive gradients of the features that had been inactived by ReLU
   */
  void ReLUActivationDerivative(galois::PerHost<pando::Array<GNNFloat>>& gradient) {
    struct Tpl {
      galois::PerHost<GNNLayerDimensions> dim;
      galois::PerHost<pando::Array<bool>> mask;
    };

    struct InnerTpl {
      pando::Array<bool> mask;
      pando::Array<GNNFloat> grad;
    };

#if 0
    std::cout << "************** RELUACTIVATION ***********\n";
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      pando::Array<GNNFloat> in = fmap(gradient, get, host);
      for (std::uint64_t i = 0; i < in.size(); ++i) {
        std::cout << "host:" << host << ", i:" << i << ", in:" << in[i] << "\n";
      }
    }
#endif

    galois::doAll(
        Tpl{this->dimensions_, this->reluActivation_}, gradient,
        +[](Tpl tpl, pando::Array<GNNFloat> grad) {
          std::uint32_t host = pando::getCurrentPlace().node.id;

          GNNLayerDimensions dim = fmap(tpl.dim, get, host);
          LayerDimension outMatDim = dim.outputRows * dim.outputColumns;
          pando::Array<bool> mask = fmap(tpl.mask, get, host);

          galois::doAll(
              InnerTpl{mask, grad}, galois::IotaRange(0, outMatDim),
              +[](InnerTpl tpl, LayerDimension i) {
                // ReLU inactivated this feature, and so does not reflect its gradient
                if (!tpl.mask[i]) {
                  tpl.grad[i] = 0;
                }
              });
        });
#if 0
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      pando::Array<GNNFloat> in = fmap(gradient, get, host);
      pando::Array<bool> mask = fmap(this->reluActivation_, get, host);
      for (std::uint64_t i = 0; i < in.size(); ++i) {
        std::cout << "host:" << host << ", i:" << i << ", mask:" <<
            mask[i] <<  ", out:" << in[i] << "\n";
      }
    }
#endif
  }

  /**
   * @brief Drop derivatives of the elements that have dropped out from training,
   * testing, or validation.
   */
  void DoDropoutDerivative() {
    struct Tpl {
      galois::PerHost<GNNLayerDimensions> dim;
      galois::PerHost<pando::Array<bool>> mask;
    };

    struct InnerTpl {
      pando::Array<GNNFloat> outMat;
      pando::Array<bool> mask;
    };

    galois::doAll(
        Tpl{this->dimensions_, this->dropoutMask_}, this->backwardOutputMatrix_,
        +[](Tpl tpl, pando::Array<GNNFloat> outMat) {
          std::uint32_t host = pando::getCurrentPlace().node.id;

          GNNLayerDimensions dim = fmap(tpl.dim, get, host);
          pando::Array<bool> mask = fmap(tpl.mask, get, host);

          LayerDimension inMatDim = dim.inputColumns * dim.inputRows;

          galois::doAll(
              InnerTpl{outMat, mask}, galois::IotaRange(0, inMatDim),
              +[](InnerTpl tpl, LayerDimension i) {
                if (tpl.mask[i]) {
                  tpl.outMat[i] = tpl.outMat[i] * 2;
                } else {
                  tpl.outMat[i] = 0;
                }
              });
        });
  }

  /**
   * @brief After gradient descent, optimizes the current layer's weight matrix.
   */
  void OptimizeLayer(AdamOptimizer optimizer, std::uint32_t layerNumber) {
    optimizer.GradientDescent(this->dimensions_, this->layerWeightGradients_, this->layerWeights_,
                              layerNumber);
  }

  /**
   * @brief The number of vertices is changed after graph sampling.
   * This method adjusts the update to a layer dimension.
   * (Without this, each phase would use dummy values to calculate inference
   *  and gradient descent)
   */
  void ResizeRowDimension(galois::PerHost<VertexDenseID> newRowDim) {
    galois::doAll(
        newRowDim, this->dimensions_,
        +[](galois::PerHost<VertexDenseID> newRowDim, pando::GlobalRef<GNNLayerDimensions> dim) {
          VertexDenseID newRow = fmap(newRowDim, get, pando::getCurrentPlace().node.id);
          GNNLayerDimensions newDim = dim;
          newDim.inputRows = newRow;
          newDim.outputRows = newRow;
          dim = newDim;
        });
  }

protected:
  /// @brief Layer ID starting from 0
  std::uint32_t layerNumber_;
  /// @brief Per-host forward output matrices
  galois::PerHost<pando::Array<GNNFloat>> forwardOutputMatrix_;
  /// @brief Per-host backward output matrices
  galois::PerHost<pando::Array<GNNFloat>> backwardOutputMatrix_;
  /// @brief Per-host input/output dimensions
  galois::PerHost<GNNLayerDimensions> dimensions_;
  /// @brief Per-host weight matrices
  galois::PerHost<pando::Array<GNNFloat>> layerWeights_;
  /// @brief Per-host weight gradient matrices
  galois::PerHost<pando::Array<GNNFloat>> layerWeightGradients_;
  /// @brief It is true if this layer requires weight (e.g., GCN)
  bool needWeight_{false};
  /// @brief Per-host dropout mask matrices
  galois::PerHost<pando::Array<bool>> dropoutMask_;
  /// @brief Random number generator for dropout
  RandomNumberGenerator dropoutSampler_;
  /// @brief Per-host ReLU activation matrices
  galois::PerHost<pando::Array<bool>> reluActivation_;
};

} // namespace gnn

#endif // PANDO_WF1_GALOIS_LAYERS_LAYER_HPP_
