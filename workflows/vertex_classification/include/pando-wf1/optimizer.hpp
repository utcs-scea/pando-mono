// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF1_OPTIMIZER_HPP_
#define PANDO_WF1_OPTIMIZER_HPP_

#include <pando-rt/containers/vector.hpp>

#include <pando-wf1/gnntypes.hpp>
#include <pando-wf1/layers/layer.hpp>

#include <pando-lib-galois/containers/host_indexed_map.hpp>

namespace gnn {

/**
 * @brief Adam optimizer.
 */
class AdamOptimizer {
public:
  /// @brief Struct for specifying adam optimizer config.
  /// Defaults based on the Adam paper.
  struct AdamConfiguration {
    GNNFloat alpha{0.01};
    GNNFloat beta1{0.9};
    GNNFloat beta2{0.999};
    GNNFloat epsilon{1e-8};
  };

  AdamOptimizer() noexcept = default;
  ~AdamOptimizer() noexcept = default;
  constexpr AdamOptimizer(AdamOptimizer&&) noexcept = default;
  constexpr AdamOptimizer(const AdamOptimizer&) noexcept = default;
  constexpr AdamOptimizer& operator=(const AdamOptimizer&) noexcept = default;
  constexpr AdamOptimizer& operator=(AdamOptimizer&&) noexcept = default;

  /**
   * @brief Allocate and initialize parameter arrays.
   */
  void initialize(pando::Vector<std::uint64_t>& trainableLayerSizes,
                  std::uint32_t numTrainableLayers) {
    // Use default configurations
    this->config_ = AdamConfiguration();

    // Each layer uses separate configurations
    PANDO_CHECK(this->firstMoments_.initialize());
    PANDO_CHECK(this->secondMoments_.initialize());
    PANDO_CHECK(this->beta1Power_.initialize());
    PANDO_CHECK(this->beta2Power_.initialize());

    struct Tpl {
      galois::HostIndexedMap<pando::Array<pando::Array<GNNFloat>>> sm;
      galois::HostIndexedMap<pando::Array<GNNFloat>> b1;
      galois::HostIndexedMap<pando::Array<GNNFloat>> b2;
      std::uint32_t numLayers;
      pando::Vector<LayerDimension> optDim;
      AdamConfiguration config;
    };

    galois::doAll(
        Tpl{this->secondMoments_, this->beta1Power_, this->beta2Power_, numTrainableLayers,
            trainableLayerSizes, this->config_},
        this->firstMoments_, +[](Tpl tpl, pando::Array<pando::Array<GNNFloat>> fmRef) {
          std::uint32_t host = pando::getCurrentPlace().node.id;

          pando::GlobalRef<pando::Array<pando::Array<GNNFloat>>> smRef = *fmap(tpl.sm, get, host);
          pando::GlobalRef<pando::Array<GNNFloat>> b1Ref = *fmap(tpl.b1, get, host);
          pando::GlobalRef<pando::Array<GNNFloat>> b2Ref = *fmap(tpl.b2, get, host);

          std::uint32_t numLayers = tpl.numLayers;

          PANDO_CHECK(fmap(fmRef, initialize, numLayers));
          PANDO_CHECK(fmap(smRef, initialize, numLayers));
          PANDO_CHECK(fmap(b1Ref, initialize, numLayers));
          PANDO_CHECK(fmap(b2Ref, initialize, numLayers));

          pando::Vector<LayerDimension> optDim = tpl.optDim;

          pando::Array<pando::Array<GNNFloat>> fm = fmRef;
          pando::Array<pando::Array<GNNFloat>> sm = smRef;

          AdamConfiguration config = tpl.config;

          for (std::uint32_t l = 0; l < numLayers; ++l) {
            pando::GlobalRef<pando::Array<GNNFloat>> ifmRef = fm[l];
            pando::GlobalRef<pando::Array<GNNFloat>> ismRef = sm[l];
            LayerDimension dim = optDim[l];
            PANDO_CHECK(fmap(ifmRef, initialize, dim));
            PANDO_CHECK(fmap(ismRef, initialize, dim));

            pando::Array<GNNFloat> ifm = ifmRef;
            pando::Array<GNNFloat> ism = ismRef;

            galois::doAll(
                ifm, +[](pando::GlobalRef<GNNFloat> v) {
                  v = 0;
                });
            galois::doAll(
                ism, +[](pando::GlobalRef<GNNFloat> v) {
                  v = 0;
                });
          }

          pando::Array<GNNFloat> b1 = b1Ref;
          pando::Array<GNNFloat> b2 = b2Ref;
          galois::doAll(
              config, b1, +[](AdamConfiguration cfg, pando::GlobalRef<GNNFloat> v) {
                v = cfg.beta1;
              });
          galois::doAll(
              config, b2, +[](AdamConfiguration cfg, pando::GlobalRef<GNNFloat> v) {
                v = cfg.beta2;
              });
        });
  }

  /**
   * @brief Update weight matrices.
   */
  void GradientDescent(galois::HostIndexedMap<GNNLayerDimensions> dim,
                       galois::HostIndexedMap<pando::Array<GNNFloat>>& derivatives,
                       galois::HostIndexedMap<pando::Array<GNNFloat>>& inputMatrix,
                       std::uint32_t layerNumber) {
    struct Tpl {
      galois::HostIndexedMap<pando::Array<pando::Array<GNNFloat>>> fm;
      galois::HostIndexedMap<pando::Array<pando::Array<GNNFloat>>> sm;
      galois::HostIndexedMap<pando::Array<GNNFloat>> inGradMat;
      galois::HostIndexedMap<pando::Array<GNNFloat>> b1p;
      galois::HostIndexedMap<pando::Array<GNNFloat>> b2p;
      galois::HostIndexedMap<GNNLayerDimensions> dim;
      AdamConfiguration config;
      std::uint32_t layerNumber;
    };

    struct InnerTpl {
      AdamConfiguration config;
      pando::Array<GNNFloat> inMat;
      pando::Array<GNNFloat> inGradMat;
      pando::Array<GNNFloat> ifm;
      pando::Array<GNNFloat> ism;
      GNNFloat b1p;
      GNNFloat b2p;
    };

    galois::doAll(
        Tpl{this->firstMoments_, this->secondMoments_, derivatives, this->beta1Power_,
            this->beta2Power_, dim, this->config_, layerNumber},
        inputMatrix, +[](Tpl tpl, pando::GlobalRef<pando::Array<GNNFloat>> inMatRef) {
          std::uint32_t host = pando::getCurrentPlace().node.id;

          std::uint32_t l = tpl.layerNumber;
          pando::Array<pando::Array<GNNFloat>> fm = *fmap(tpl.fm, get, host);
          pando::Array<pando::Array<GNNFloat>> sm = *fmap(tpl.sm, get, host);
          pando::Array<GNNFloat> ifm = fm[l];
          pando::Array<GNNFloat> ism = sm[l];
          pando::Array<GNNFloat> b1pArr = *fmap(tpl.b1p, get, host);
          pando::Array<GNNFloat> b2pArr = *fmap(tpl.b1p, get, host);
          GNNFloat b1p = b1pArr[l];
          GNNFloat b2p = b2pArr[l];
          pando::Array<GNNFloat> inGradMat = *fmap(tpl.inGradMat, get, host);
          GNNLayerDimensions dim = *fmap(tpl.dim, get, host);
          LayerDimension inGradMatDim = dim.inputColumns * dim.outputColumns;

          galois::doAll(
              InnerTpl{tpl.config, inMatRef, inGradMat, ifm, ism, b1p, b2p},
              galois::IotaRange(0, inGradMatDim), +[](InnerTpl tpl, LayerDimension i) {
                AdamConfiguration config = tpl.config;
                // weight decay:
                tpl.inGradMat[i] += (5e-4) * tpl.inMat[i];

                tpl.ifm[i] = config.beta1 * tpl.ifm[i] + (1.0 - config.beta1) * tpl.inGradMat[i];
                tpl.ism[i] = config.beta2 * tpl.ism[i] +
                             (1.0 - config.beta2) * (tpl.inGradMat[i] * tpl.inGradMat[i]);
                GNNFloat fbiasCorrect = tpl.ifm[i] / (1.0 - tpl.b1p);
                GNNFloat sbiasCorrect = tpl.ism[i] / (1.0 - tpl.b2p);

                if (std::sqrt(sbiasCorrect) + config.epsilon != 0) {
                  tpl.inMat[i] -=
                      config.alpha * fbiasCorrect / (std::sqrt(sbiasCorrect) + config.epsilon);
                }
              });

          b1pArr[l] *= tpl.config.beta1;
          b2pArr[l] *= tpl.config.beta2;
        });
  }

private:
  /// @brief Adam optimizer configurations
  AdamConfiguration config_;
  /// @brief Adaptive Adam optimizer parameters
  galois::HostIndexedMap<pando::Array<pando::Array<GNNFloat>>> firstMoments_;
  galois::HostIndexedMap<pando::Array<pando::Array<GNNFloat>>> secondMoments_;
  galois::HostIndexedMap<pando::Array<GNNFloat>> beta1Power_;
  galois::HostIndexedMap<pando::Array<GNNFloat>> beta2Power_;
};

} // namespace gnn

#endif // PANDO_WF1_OPTIMIZER_HPP_
