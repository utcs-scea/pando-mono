// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF1_GALOIS_GNNTYPES_HPP_
#define PANDO_WF1_GALOIS_GNNTYPES_HPP_

#include <random>

#include <pando-rt/memory/global_ptr.hpp>

#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/containers/per_thread.hpp>
#include <pando-lib-galois/loops/do_all.hpp>

/** @file gnntypes.hpp
 *  This file declares types that will be used across the entire GNN codes.
 */

namespace gnn {
/// @brief Floating point type to use through GNN computation
using GNNFloat = float;

/// @brief Layer dimension (row, column) type
using LayerDimension = std::uint64_t;

/// @brief Vertex global/local dense ID type
using VertexDenseID = std::uint64_t;

/// @brief Edge global/local dense ID type
using EdgeDenseID = std::uint64_t;

/**
 * @brief Struct holding the dimension of a layer. A layer takes an input matrix and outputs another
 * another matrix with a different number of columns (e.g., an input matrix multiplies with a set of
 * weight matrices).
 */
struct GNNLayerDimensions {
  /// Number of rows in input (and maybe output) of this layer
  LayerDimension inputRows;
  /// Number of columns in input of this layer
  LayerDimension inputColumns;
  /// Number of columns output of this layer
  LayerDimension outputColumns;
  /// If the number of rows changes, this is set. Otherwise, ignored.
  LayerDimension outputRows;
};

/**
 * @brief Helper struct to keep start/end/size of any particular range.
 * This is mostly used for vertex type ranges.
 */
struct GNNRange {
  std::uint64_t begin{0};
  std::uint64_t end{0};
  std::uint64_t size{0};
};

/**
 * @brief Phase of GNN computation.
 */
enum class GNNPhase { kTrain, kValidate, kTest, kOther, kBatch };

/**
 * @brief This is per-thread random number generator.
 */
class PerThreadRNGArray {
public:
  PerThreadRNGArray() {}
  void initialize(pando::Vector<galois::PlaceType> nodeList) {
    // Calculate the total number of FGMT threads on this system.
    pando::Place placeDims = pando::getPlaceDims();
    std::uint16_t numNodes = placeDims.node.id;
    std::uint8_t numPods = placeDims.pod.x * placeDims.pod.y * numNodes;
    std::uint64_t numCores = placeDims.core.x * placeDims.core.y * numPods;
    // Initilaize distributed arrays of random number distribution and generating engine.
    // Each index is corresponding to an owner thread id.
    PANDO_CHECK(this->engine_.initialize(nodeList.begin(), nodeList.end(), numCores));
    PANDO_CHECK(this->distribution_.initialize(nodeList.begin(), nodeList.end(), numCores));
    // TODO(hc): temporarily store thread ids to iterate
    PANDO_CHECK(this->threadIDs_.initialize(nodeList.begin(), nodeList.end(), numCores));
    for (std::uint64_t tid = 0; tid < this->threadIDs_.size(); ++tid) {
      this->threadIDs_[tid] = tid;
    }
    // Tuple to pack necessary operands for the following doAll
    struct Tpl {
      std::uint64_t numCores;
      galois::DistArray<std::default_random_engine> engine;
      galois::DistArray<std::uniform_real_distribution<GNNFloat>> distributions;
    };
    std::cout << "Total number of cores;" << numCores << "\n";
    galois::doAll(
        Tpl{numCores, this->engine_, this->distribution_}, this->threadIDs_,
        +[](Tpl tpl, std::uint64_t tid) {
          std::uint64_t numCores = tpl.numCores;
          std::uniform_real_distribution<GNNFloat> distribution = tpl.distributions[tid];
          distribution = std::uniform_real_distribution<GNNFloat>(0.0, 1.0);
          std::default_random_engine engine = tpl.engine[tid];
          engine.seed(tid * numCores);
          // engine.seed(std::chrono::system_clock::now().time_since_epoch().count());
          //  The above engine and distribution are copied instead of referenced, and
          //  so, these should be copied back to the original ones; otherwise, it does
          //  not produce random values, but always 0 or 1.
          tpl.engine[tid] = engine;
          tpl.distributions[tid] = distribution;
        });
  }

  /**
   * @brief Get the number of thread id of the current context.
   */
  std::uint64_t getThreadID() {
    pando::Place placeDims = pando::getPlaceDims();
    pando::Place currentPlace = pando::getCurrentPlace();
    std::uint64_t numCoresPerPod = placeDims.core.x * placeDims.core.y;
    std::uint64_t numCoresPerNode = numCoresPerPod * placeDims.pod.x * placeDims.pod.y;
    std::uint64_t tid = /* The current PXN's core offset */
        currentPlace.node.id * numCoresPerNode +
        /* The current POD's core offset */
        ((currentPlace.pod.x * placeDims.pod.y) + currentPlace.pod.y) * numCoresPerPod +
        ((currentPlace.core.x * placeDims.core.y) + currentPlace.core.y);
    // Increase tid by 1 to avoid a seed number 0
    return tid + 1;
  }

  /**
   * @brief Generate a random number by using per-thread random number generation engine.
   */
  GNNFloat GetRandomNumber() {
    std::uint64_t tid = this->getThreadID();
    std::uniform_real_distribution<GNNFloat> currentDistribution = this->distribution_[tid];
    std::default_random_engine currentEngine = this->engine_[tid];
    GNNFloat randomNumber = currentDistribution(currentEngine);
    // TODO(hc): I am not sure if this is necessary.
    // If random distribution and engine maintain their internal states for distribution
    // these copy backs are required.
    this->distribution_[tid] = currentDistribution;
    this->engine_[tid] = currentEngine;
    return randomNumber;
  }

  /**
   * @brief Return true or false based on the given threshold rate.
   * This is used for drop-out of the layer.
   */
  bool DoBernoulli(float threshold) {
    return (this->GetRandomNumber() > threshold) ? true : false;
  }

private:
  /// Random generating engine and distribution
  galois::DistArray<std::default_random_engine> engine_;
  galois::DistArray<std::uniform_real_distribution<GNNFloat>> distribution_;
  // TODO(hc): This will be replaced with counted iterator
  galois::DistArray<std::uint64_t> threadIDs_;
};

/**
 * @brief This is random number generator.
 */
class RandomNumberGenerator {
public:
  RandomNumberGenerator() {}

  /**
   * @brief Generate a random number by using per-thread random number generation engine.
   */
  GNNFloat GetRandomNumber() {
    std::mt19937_64 currentEngine(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<GNNFloat> currentDistribution(0.0, 1.0);
    GNNFloat randomNumber = currentDistribution(currentEngine);
    return randomNumber;
  }

  /**
   * @brief Return true or false based on the given threshold rate.
   * This is used for drop-out of the layer.
   */
  bool DoBernoulli(float threshold) {
    return (this->GetRandomNumber() > threshold) ? true : false;
  }
};

} // namespace gnn

#endif // PANDO_WF1_GALOIS_GNNTYPES_HPP_
