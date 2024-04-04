// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF1_GALOIS_GNN_HPP_
#define PANDO_WF1_GALOIS_GNN_HPP_

#include <utility>

#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/loops/do_all.hpp>

#include <pando-rt/containers/vector.hpp>

#include <pando-wf1-galois/gnntypes.hpp>
#include <pando-wf1-galois/graphs/gnngraph.hpp>
#include <pando-wf1-galois/layers/fcn.hpp>
#include <pando-wf1-galois/layers/gcn.hpp>
#include <pando-wf1-galois/layers/layer.hpp>
#include <pando-wf1-galois/layers/softmax.hpp>
#include <pando-wf1-galois/optimizer.hpp>

namespace gnn {

/**
 * @brief Class that manages overall graph neural network based training, testing, and validation.
 *
 * This class is the core of the graph neural network (GNN) workflow, and manages all training,
 * testing, and validation phases. This class aggregates and orchestrates building blocks of GNN
 * including neural network layers, activation layers, non-linear layers, an optimizer, and a graph.
 *
 * This class supports vertex classification, link prediction, and mulit-hop reasoning training.
 *
 */
template <typename Graph>
class GraphNeuralNetwork {
public:
  constexpr GraphNeuralNetwork() noexcept = default;

  void initialize(pando::GlobalPtr<Graph> dGraphPtr) {
    this->numPXNs = pando::getPlaceDims().node.id;
    PANDO_CHECK(this->nodeList_.initialize(this->numPXNs));
    for (std::int16_t n = 0; n < this->numPXNs; ++n) {
      this->nodeList_[n] =
          galois::PlaceType{pando::Place{pando::NodeIndex{n}, pando::anyPod, pando::anyCore},
                            pando::MemoryType::Main};
    }
    this->gnnGraphPtr_ = static_cast<decltype(this->gnnGraphPtr_)>(
        pando::getDefaultMainMemoryResource()->allocate(sizeof(graph::GNNGraph<Graph>)));
    this->gnnGraphPtr_->initialize(dGraphPtr);
    this->InitializeLayers(dGraphPtr);
    // Setup optimizers
    pando::Vector<LayerDimension> optimizerSizes;
    PANDO_CHECK(optimizerSizes.initialize(0));
    PANDO_CHECK(optimizerSizes.pushBack(this->gnnGraphPtr_->VertexFeatureLength() * 16));
    PANDO_CHECK(optimizerSizes.pushBack(16 * this->gnnGraphPtr_->GetNumClasses()));
    this->optimizer_.initialize(optimizerSizes, this->numGCNLayers_);
  }

  ~GraphNeuralNetwork() {
    for (std::uint32_t l = 0; l < this->numGCNLayers_; ++l) {
      pando::getDefaultMainMemoryResource()->deallocate(
          static_cast<pando::GlobalPtr<GraphConvolutionalLayer<Graph>>>(this->gcnLayers_[l]),
          sizeof(GraphConvolutionalLayer<Graph>));
    }
  }

  /**
   * @brief Allocates and initialize layers.
   */
  void InitializeLayers(pando::GlobalPtr<Graph> dGraphPtr) {
    galois::PerHost<pando::Array<GNNFloat>> backwardOutputMatrix;
    struct Tpl {
      LayerDimension inColumnLen;
      LayerDimension outColumnLen;
      pando::GlobalPtr<Graph> gPtr;
    };

    /***************************************************************************
     *                        GCN initialization                           *
     ***************************************************************************/

    PANDO_CHECK(this->gcnLayers_.initialize(this->numGCNLayers_));
    LayerDimension inColumnLen{0}, outColumnLen{0};
    // Setup GCNlayers with AGILE's configurations
    for (std::uint32_t l = 0; l < this->numGCNLayers_; ++l) {
      // ** [Dimension] **
      //
      // ** 1st GCN Layer:
      // Input dimension: (# of sampled vertices) x 30
      // Output dimension: (# of sampled vertices) x 16
      //
      // ** 2nd GCN Layer:
      // Input dimension: (# of sampled vertices) x 16
      // Output dimension: (# of sampled vertices) x 5

      if (l == 0) {
        inColumnLen = this->gnnGraphPtr_->VertexFeatureLength();
        outColumnLen = 16;
      } else if (l == 1) {
        inColumnLen = 16;
        outColumnLen = this->gnnGraphPtr_->GetNumClasses();
      }

      // Per-host layer dimension; Each PXN materializes and uses different matrices
      // with different dimensions
      galois::PerHost<gnn::GNNLayerDimensions> dimension;
      PANDO_CHECK(dimension.initialize());
      galois::doAll(
          Tpl{inColumnLen, outColumnLen, dGraphPtr}, dimension,
          +[](Tpl tpl, pando::GlobalRef<gnn::GNNLayerDimensions> dim) {
            std::uint32_t host = pando::getCurrentPlace().node.id;

            // Sampled graph size changes for each minibatch.
            // To avoid matrix reconstruction for each minibatch,
            // allocates matrices once with the original graph size, and
            // reuses them.

            VertexDenseID localSize = fmap(*(tpl.gPtr), localSize, host);
            gnn::GNNLayerDimensions nDim = {localSize, tpl.inColumnLen, tpl.outColumnLen,
                                            localSize};
            dim = nDim;
            // std::cout << host << " GCN layer dimension: (" << localSize << "x" << tpl.inColumnLen
            //           << ") * (" << localSize << "x" << tpl.outColumnLen << ")\n";
          });

      this->gcnLayers_[l] = static_cast<pando::GlobalPtr<GraphConvolutionalLayer<Graph>>>(
          pando::getDefaultMainMemoryResource()->allocate(sizeof(GraphConvolutionalLayer<Graph>)));
      pando::GlobalPtr<GraphConvolutionalLayer<Graph>> gcnPtr = this->gcnLayers_[l];
      gcnPtr->initialize(l, inColumnLen, backwardOutputMatrix, dimension);
      // The forward output matrix is revisited and updated during its backward phase
      backwardOutputMatrix = gcnPtr->GetForwardOutputMatrix();
    }

    /***************************************************************************
     *                        Softmax initialization                           *
     ***************************************************************************/

    std::uint64_t numClasses = this->gnnGraphPtr_->GetNumClasses();
    inColumnLen = numClasses;
    outColumnLen = numClasses;

    // Per-host layer dimension; Each PXN materializes and uses different matrices
    // with different dimensions
    galois::PerHost<gnn::GNNLayerDimensions> sfMaxDim;
    PANDO_CHECK(sfMaxDim.initialize());
    galois::doAll(
        Tpl{inColumnLen, outColumnLen, dGraphPtr}, sfMaxDim,
        +[](Tpl tpl, pando::GlobalRef<gnn::GNNLayerDimensions> dim) {
          std::uint32_t host = pando::getCurrentPlace().node.id;

          // Sampled graph size changes for each minibatch.
          // To avoid matrix reconstruction for each minibatch,
          // allocates matrices once with the original graph size, and
          // reuses them.

          VertexDenseID localSize = fmap(*(tpl.gPtr), localSize, host);
          gnn::GNNLayerDimensions nDim = {localSize, tpl.inColumnLen, tpl.outColumnLen, localSize};
          dim = nDim;
          // std::cout << host << " Softmax layer dimension: (" << localSize << "x" <<
          // tpl.inColumnLen
          //           << ") * (" << localSize << "x" << tpl.outColumnLen << ")\n";
        });

    this->softmaxLayer_ = static_cast<decltype(this->softmaxLayer_)>(
        pando::getDefaultMainMemoryResource()->allocate(sizeof(SoftmaxLayer<Graph>)));
    this->softmaxLayer_->initialize(this->numGCNLayers_, backwardOutputMatrix, sfMaxDim);
  }

  /**
   * @brief Start a training phase with the specified epochs.
   */
  float Train(std::uint64_t numEpochs) {
    float trainAccuracy{0.f};
    // Allocate necessary arrays for graph sampling
    this->gnnGraphPtr_->InitializePerHostGraphSampling();
    for (std::uint64_t epoch = 0; epoch < numEpochs; ++epoch) {
      // Reset a minibatcher:
      // A minibatcher stores vertex IDs for training into a vector.
      // To randomly choose and train a subset of the vertices for training,
      // it shuffles the vector for each epoch, and chooses one chunk by one
      // chunk as a minibatch.
      this->gnnGraphPtr_->ResetTrainMinibatch();
      std::uint64_t minibatchEpoch{0};
      double minibatchCorrectSum{0}, minibatchCheckSum{0};
      while (true) {
        std::cout << "[Epoch: " << epoch << " | Minibatch Epoch: " << minibatchEpoch << "]\n";
        ++minibatchEpoch;
        // Reset a GNN graph state for the next graph sampling
        this->gnnGraphPtr_->ResetSamplingState();
        // Choose the next minibatch for training
        VertexDenseID seedVertices = this->gnnGraphPtr_->PrepareNextTrainMinibatch();
        // Sample edges and vertices from seed vertices
        // Construct a subgraph with them
        galois::PerHost<VertexDenseID> numSampledVertices = this->gnnGraphPtr_->SampleEdges();
        this->CorrectRowCounts(numSampledVertices);
        // Start the inference phase
        galois::PerHost<pando::Array<GNNFloat>> forwardOutput = this->DoInference();
        // Calculate accuracy
        std::pair<VertexDenseID, VertexDenseID> accuracyPair =
            this->gnnGraphPtr_->GetGlobalAccuracy(forwardOutput, GNNPhase::kBatch);
        minibatchCorrectSum += accuracyPair.second;
        minibatchCheckSum += accuracyPair.first;
        // Gradient propagation
        this->GradientPropagation();
        // Check if all vertices and edges are minibatched and used for training
        if (this->gnnGraphPtr_->NoMoreTrainMinibatching()) {
          break;
        }
      }
      std::cout << "--> Correct:" << minibatchCorrectSum << ", Total:" << minibatchCheckSum
                << ", Accuracy:" << minibatchCorrectSum / minibatchCheckSum << "\n";
    }
    return trainAccuracy;
  }

#if 0
  void Test(uint64_t epoch) {
    // Reset a minibatcher pointer and shuffle vertex index range
    this->gnnGraphPtr_->ResetTestMinibatch();
    std::uint64_t minibatchEpoch{0};
    float minibatchCorrectSum{0}, minibatchCheckSum{0};
    while (true) {
      std::cout << "Test Epoch:" << epoch << ", minibatch epoch:" << minibatchEpoch << "\n";
      ++minibatchEpoch;
      std::uint32_t numSampledLayers{0};

      // TODO(hc): Would be great if this is reverse iterators but polymorphism
      // didn't work as I expected.
      // gnnGraph.PrepareNeighborhoodSample();

      // Get next minibatch
      std::uint64_t seedVertices = this->gnnGraphPtr_->PrepareNextTestMinibatch();

      if (!this->samplingForEachlayer) {
        // From seed vertices, sample edges and their destinations
        std::uint64_t numSampledVertices = this->gnnGraphPtr_->SampleEdgesForAllLayers();
      } else {
        VertexTopologyID numTargetSampledVertices{10};
        for (size_t i = this->gcnLayers_.size(); i > 0; i--) {
          if (i == 1) {
            numTargetSampledVertices = 25;
          }
          pando::GlobalPtr<GraphConvolutionalLayer<Graph>> gcn =
              this->gcnLayers_[i - 1];
          std::cout << "Layer " << i << ", " << numSampledLayers + 1 << "\n";
          std::uint64_t numSampledVertices = this->gnnGraphPtr_->SampleEdges(
              gcn->GetLayerNumber(), numTargetSampledVertices, numSampledLayers + 1);
          std::cout << "GCN layer graph sampling completes:" << gcn->GetLayerNumber()
                    << "'s sampled vertices:" << numSampledVertices << "\n";
          if (numSampledVertices == 0) {
            std::cout << "Sampled graph size is 0, and so exit it.\n";
            pando::exit(EXIT_FAILURE);
          }
          ++numSampledLayers;
        }
      }
      // Graph sampling changes the number of vertices (so, the number of vertices
      // in a subgraph), and so the layers should change its dimension properly
      this->CorrectRowCounts(this->gnnGraphPtr_->ConstructSubgraph(this->numGCNLayers_));

      // Start forward phase
      galois::DistArray<GNNFloat> forwardOutput = this->DoInference();
      // Calculate accuracy
      std::pair<VertexTopologyID, VertexTopologyID> accuracyPair =
          this->gnnGraphPtr_->GetGlobalAccuracy(forwardOutput, GNNPhase::kBatch);
      minibatchCorrectSum += accuracyPair.second;
      minibatchCheckSum += accuracyPair.first;
      // Check if all vertices and edges are minibatched and used for training
      if (this->gnnGraphPtr_->NoMoreTestMinibatching()) {
        break;
      }
    }
    std::cout << "Test Epoch:" << epoch << ", Correct:" << minibatchCorrectSum
              << ", Total:" << minibatchCheckSum
              << ", Accuracy:" << minibatchCorrectSum / minibatchCheckSum << "\n";
  }
#endif

private:
  /**
   * @brief Perform inference phases across layers.
   *
   * @details This method consecutively performs the forward phase for each layer.
   * An output of the layer is an input of the next layer.
   */
  galois::PerHost<pando::Array<GNNFloat>> DoInference() {
    galois::PerHost<pando::Array<GNNFloat>> layerInput;
    for (uint32_t l = 0; l < this->gcnLayers_.size(); ++l) {
      pando::GlobalPtr<GraphConvolutionalLayer<Graph>> gcn = this->gcnLayers_[l];
      bool isLastLayer = (l == this->gcnLayers_.size() - 1) ? true : false;
      if (l == 0) {
        // Input matrix of the first GCN layer is the input vertex features
        layerInput = gcn->ForwardPhase(this->gnnGraphPtr_, isLastLayer);
      } else {
        // Input matrices for the other GCN layers are the output of the previous layer
        layerInput = gcn->ForwardPhase(layerInput, this->gnnGraphPtr_, isLastLayer);
      }
    }

    return this->softmaxLayer_->ForwardPhase(layerInput, this->gnnGraphPtr_);
  }

  /**
   * @brief This method consecutviely performs the backward phase for each layer
   * in a reverse order from the forward phase.
   */
  void GradientPropagation() {
    // Calculate softmax gradient
    galois::PerHost<pando::Array<GNNFloat>> prevLayerGradient =
        this->softmaxLayer_->BackwardPhase(this->gnnGraphPtr_);
    for (std::uint32_t l = this->gcnLayers_.size(); l > 0; l--) {
      pando::GlobalPtr<GraphConvolutionalLayer<Graph>> gcn = this->gcnLayers_[l - 1];
      bool isLastLayer = (l == this->gcnLayers_.size()) ? true : false;
      prevLayerGradient = gcn->BackwardPhase(prevLayerGradient, this->gnnGraphPtr_, isLastLayer);
      // Perform gradient descent and update each model
      gcn->OptimizeLayer(this->optimizer_, l - 1);
    }
  }

  /**
   * @brief Graph sampling changes the number of rows for each layer.
   * This method reflects the new dimension of each layer.
   */
  void CorrectRowCounts(galois::PerHost<VertexDenseID> newRows) {
    std::uint32_t layerOffset{0};
    for (std::uint32_t l = this->gcnLayers_.size(); l > 0; l--) {
      pando::GlobalPtr<GraphConvolutionalLayer<Graph>> gcn = this->gcnLayers_[l - 1];
      gcn->ResizeRowDimension(newRows);
    }
    // The softmax layer uses all seed vertices for training
    this->softmaxLayer_->ResizeRowDimension(newRows);
  }

  /// @brief The number of PXNs
  std::int16_t numPXNs;
  /// @brief The current WF1 is working based on fork-join model,
  /// and the master PXN coordinates computation
  pando::Vector<galois::PlaceType> nodeList_;
  /// @brief GNN graph pointer to the original graph
  pando::GlobalPtr<graph::GNNGraph<Graph>> gnnGraphPtr_;
  /// @brief  Number of GCN layers
  std::uint32_t numGCNLayers_{2};
  // TODO(hc): would be better to use polymorphism if possible.
  // but I am not sure how GlobalPtr works for polymorphism and inheritance.
  /// @brief GCN layers
  pando::Vector<pando::GlobalPtr<GraphConvolutionalLayer<Graph>>> gcnLayers_;
  /// @brief Softmax layer
  pando::GlobalPtr<SoftmaxLayer<Graph>> softmaxLayer_;
  /// @brief Adam optimizer
  AdamOptimizer optimizer_;
};

} // namespace gnn

#endif // PANDO_WF1_GALOIS_GNN_HPP_
