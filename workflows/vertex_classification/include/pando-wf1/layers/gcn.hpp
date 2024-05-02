// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF1_LAYERS_GCN_HPP_
#define PANDO_WF1_LAYERS_GCN_HPP_

#include <pando-lib-galois/loops/do_all.hpp>

#include <pando-rt/containers/array.hpp>
#include <pando-rt/containers/vector.hpp>

#include <pando-wf1/graphs/gnngraph.hpp>
#include <pando-wf1/layers/layer.hpp>
#include <pando-wf1/math/gnnmath.hpp>

namespace gnn {

/**
 * @brief Graph Convolutional Network (GCN) layer.
 */
template <typename InnerGraph>
class GraphConvolutionalLayer : public GNNLayer<InnerGraph> {
public:
  using VertexTopologyID = typename InnerGraph::VertexTopologyID;
  using EdgeHandle = typename InnerGraph::EdgeHandle;
  using VertexData = typename InnerGraph::VertexData;
  using EdgeData = typename InnerGraph::EdgeData;
  using VertexRange = typename InnerGraph::VertexRange;
  using VertexIterator = typename InnerGraph::VertexIt;
  using LCSR = typename InnerGraph::CSR;

  constexpr GraphConvolutionalLayer() : GNNLayer<InnerGraph>() {}

  void initialize(std::uint32_t layerNumber, LayerDimension inColumnLen,
                  galois::PerHost<pando::Array<GNNFloat>>& backwardOutputMatrix,
                  galois::PerHost<GNNLayerDimensions>& dimensions) {
    std::cout << "[GCN Layer " << layerNumber << "] Starts initialization\n" << std::flush;
    this->inColumnLen_ = inColumnLen;
    // Call and initialize operand matrices
    GNNLayer<InnerGraph>::initialize(layerNumber, backwardOutputMatrix, dimensions, true);
    this->InitializeTempMatrices();
    std::cout << "[GCN Layer " << layerNumber << "] Starts initialization [DONE]\n" << std::flush;
  }

  constexpr GraphConvolutionalLayer(GraphConvolutionalLayer<InnerGraph>&&) noexcept = default;
  constexpr GraphConvolutionalLayer(const GraphConvolutionalLayer<InnerGraph>&) noexcept = default;
  constexpr GraphConvolutionalLayer<InnerGraph>& operator=(
      const GraphConvolutionalLayer<InnerGraph>&) noexcept = default;
  constexpr GraphConvolutionalLayer<InnerGraph>& operator=(
      GraphConvolutionalLayer<InnerGraph>&&) noexcept = default;

  /**
   * @brief Allocates operand matrices.
   */
  void InitializeTempMatrices() {
    struct Tpl {
      galois::PerHost<pando::Array<GNNFloat>> tmpin1_;
      galois::PerHost<pando::Array<GNNFloat>> tmpin2_;
      galois::PerHost<pando::Array<GNNFloat>> tmpout_;
    };

    PANDO_CHECK(this->tempInputMatrix1_.initialize());
    PANDO_CHECK(this->tempInputMatrix2_.initialize());
    PANDO_CHECK(this->tempOutputMatrix_.initialize());

    galois::doAll(
        Tpl{this->tempInputMatrix1_, this->tempInputMatrix2_, this->tempOutputMatrix_},
        this->dimensions_, +[](Tpl tpl, GNNLayerDimensions dimension) {
          std::uint32_t host = pando::getCurrentPlace().node.id;

          // std::cout << "Dimension:" << dimension.inputRows << "x" << dimension.inputColumns;
          // std::cout << ", " << dimension.outputRows << "x" << dimension.outputColumns << "\n";

          LayerDimension inDim = dimension.inputRows * dimension.inputColumns;
          LayerDimension gradDim = dimension.inputRows * dimension.outputColumns;

          pando::GlobalRef<pando::Array<GNNFloat>> tmpin1 = fmap(tpl.tmpin1_, get, host);
          pando::GlobalRef<pando::Array<GNNFloat>> tmpin2 = fmap(tpl.tmpin2_, get, host);
          pando::GlobalRef<pando::Array<GNNFloat>> tmpout = fmap(tpl.tmpout_, get, host);

          PANDO_CHECK(fmap(tmpin1, initialize, inDim));
          PANDO_CHECK(fmap(tmpin2, initialize, inDim));
          PANDO_CHECK(fmap(tmpout, initialize, gradDim));
        });
  }

  /**
   * @brief Start the forward phase of the first GCN layer.
   *
   * @details The first GCN layer and other GCN layers use different data structures
   * for vertex embedding.
   * The first GCN layer aggregates initial vertex features associated with the graph
   * topology. Other GCN layers use vertex embedding in separate GNNFloat arrays.
   * This method is for the first GCN layer.
   */
  const galois::PerHost<pando::Array<GNNFloat>> ForwardPhase(
      pando::GlobalPtr<graph::GNNGraph<InnerGraph>> gPtr, bool isLastGCN) {
    this->AggregateEmbeddings_0(this->tempInputMatrix2_, gPtr);
    this->UpdateEmbedding(this->tempInputMatrix2_, this->forwardOutputMatrix_);
    // If the current layer is the last GCN layer, this is not activated for the output layer
    this->ReLUActivation();
    return this->forwardOutputMatrix_;
  }

  /**
   * @brief Start the forward phase of the GCN layer which is not the first GCN layer.
   */
  const galois::PerHost<pando::Array<GNNFloat>> ForwardPhase(
      galois::PerHost<pando::Array<GNNFloat>>& inputEmbeddings,
      pando::GlobalPtr<graph::GNNGraph<InnerGraph>> gPtr, bool isLastGCN) {
    // This follows the AGILE GNN's implementation.
    // Only the last layer applies feature dropout.
    if (isLastGCN) {
      this->DoDropout(inputEmbeddings, this->tempInputMatrix1_);
    }
    this->AggregateEmbeddings(this->tempInputMatrix1_, this->tempInputMatrix2_, gPtr);
    this->UpdateEmbedding(this->tempInputMatrix2_, this->forwardOutputMatrix_);
    if (!isLastGCN) {
      // If the current layer is the last GCN layer, this is not activated for the output layer
      this->ReLUActivation();
    }
    return this->forwardOutputMatrix_;
  }

  /**
   * @brief Start the backward phase of the GCN layer.
   */
  galois::PerHost<pando::Array<GNNFloat>> BackwardPhase(
      galois::PerHost<pando::Array<GNNFloat>>& inputGradients,
      pando::GlobalPtr<graph::GNNGraph<InnerGraph>> gPtr, bool isLastGCN) {
    if (!isLastGCN) {
      this->ReLUActivationDerivative(inputGradients);
    }

    // this->tempInputMatrix1_: input
    // this->tempInputMatrix2_: aggr data output
    this->CalculateWeightGradient(this->tempInputMatrix2_, inputGradients,
                                  this->layerWeightGradients_);
    if (this->layerNumber_ != 0) {
      this->CalculateLayerGradient(inputGradients, this->tempInputMatrix1_);
      this->AggregateEmbeddings(this->tempInputMatrix1_, this->backwardOutputMatrix_, gPtr);
      this->DoDropoutDerivative();
    }

    return this->backwardOutputMatrix_;
  }

  /**
   * @brief Each vertex aggregates vertex embeddings of its immediate neighborhoods.
   */
  void AggregateEmbeddings_0(galois::PerHost<pando::Array<GNNFloat>>& aggrEmbeddings,
                             pando::GlobalPtr<graph::GNNGraph<InnerGraph>> gPtr) {
    struct Tpl {
      LayerDimension columnLen;
      pando::GlobalPtr<graph::GNNGraph<InnerGraph>> gPtr;
    };

    struct InnerTpl {
      LayerDimension rowLen;
      LayerDimension columnLen;
      pando::GlobalPtr<graph::GNNGraph<InnerGraph>> gPtr;
      pando::Array<bool> subgraph;
      pando::Array<GNNFloat> outMat;
    };

#if 0
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      for (VertexTopologyID v : lift(*gPtr, Vertices)) {
        std::uint64_t vid = gPtr->GetVertexIndex(v);
        pando::GlobalRef<VertexData> vDataRef = fmap(*gPtr, GetData, v);
        VertexData vData = vDataRef;
        std::cout << vid << " embedding\n";
        for (size_t i = 0; i < this->inColumnLen_; ++i) {
          std::cout << vid << "," << i << "," << vData.embedding[i] << "\n";
        }
        vDataRef = vData;
      }

      std::cout << "subgraph print\n";
      pando::Array<bool> subgraph = gPtr->GetSubgraph(host);
      VertexDenseID subgraphSize = gPtr->GetSubgraphSize(host);
      for (VertexDenseID subvid = 0; subvid < subgraphSize; ++subvid) {
        std::cout << subvid << "(" << gPtr->GetVIdFromSubgraphVId(host, subvid) << ")\n";
        for (LayerDimension ri = 0; ri < subgraphSize; ++ri) {
          if (subgraph[subvid * subgraphSize + ri]) {
            VertexDenseID did = fmap(*(gPtr), GetVIdFromSubgraphVId, host, ri);
            std::cout << "\t" << ri << "(" << did << ")\n";
          }
        }
      }
    }
#endif

    galois::doAll(
        Tpl{this->inColumnLen_, gPtr}, aggrEmbeddings, +[](Tpl tpl, pando::Array<GNNFloat> outMat) {
          std::uint32_t host = pando::getCurrentPlace().node.id;

          pando::Array<bool> subgraph = fmap(*(tpl.gPtr), GetSubgraph, host);
          VertexDenseID rowLen = fmap(*(tpl.gPtr), GetSubgraphSize, host);
          LayerDimension columnLen = tpl.columnLen;

          galois::doAll(
              InnerTpl{rowLen, columnLen, tpl.gPtr, subgraph, outMat}, galois::IotaRange(0, rowLen),
              +[](InnerTpl tpl, VertexDenseID row) {
                std::uint32_t host = pando::getCurrentPlace().node.id;

                LayerDimension rowLen = tpl.rowLen;
                LayerDimension columnLen = tpl.columnLen;

                // Reset the output matrix
                for (LayerDimension f = 0; f < columnLen; ++f) {
                  tpl.outMat[row * columnLen + f] = 0;
                }

                // Aggregate adjacent vertex embeddings
                // Note that the dimension of the adjacent matrix is (rowLen x rowLen)
                for (LayerDimension ri = 0; ri < rowLen; ++ri) {
                  if (tpl.subgraph[row * rowLen + ri]) {
                    VertexDenseID did = fmap(*(tpl.gPtr), GetVIdFromSubgraphVId, host, ri);
                    VertexTopologyID dtopoId = fmap(*(tpl.gPtr), GetTopologyIDFromIndex, did);
                    VertexData vData = fmap(*(tpl.gPtr), GetData, dtopoId);
                    pando::Array<GNNFloat> dstEmbed = vData.embedding;
                    for (LayerDimension fi = 0; fi < columnLen; ++fi) {
                      tpl.outMat[row * columnLen + fi] += dstEmbed[fi];
                    }
                  }
                }
              });
        });

#if 0
    std::cout << "GCN0 FORWARD output\n";
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      pando::GlobalRef<pando::Array<GNNFloat>> outRef = fmap(aggrEmbeddings, get, host);
      pando::Array<GNNFloat> out = outRef;
      for (std::uint64_t i = 0; i < lift(outRef, size); ++i) {
        std::cout << i << ":" << out[i] << "\n";
      }
    }
#endif
  }

  /**
   * @brief Each vertex aggregates embeddings of the immediate neighborhoods.
   */
  void AggregateEmbeddings(galois::PerHost<pando::Array<GNNFloat>>& inputEmbeddings,
                           galois::PerHost<pando::Array<GNNFloat>>& aggrEmbeddings,
                           pando::GlobalPtr<graph::GNNGraph<InnerGraph>> gPtr) {
    struct Tpl {
      LayerDimension columnLen;
      galois::PerHost<pando::Array<GNNFloat>> inMat;
      pando::GlobalPtr<graph::GNNGraph<InnerGraph>> gPtr;
    };

    struct InnerTpl {
      LayerDimension rowLen;
      LayerDimension columnLen;
      pando::GlobalPtr<graph::GNNGraph<InnerGraph>> gPtr;
      pando::Array<bool> subgraph;
      pando::Array<GNNFloat> inMat;
      pando::Array<GNNFloat> outMat;
    };

#if 0
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      pando::Array<GNNFloat> in = fmap(inputEmbeddings, get, host);
      GNNLayerDimensions dim = fmap(this->dimensions_, get, host);
      std::cout << "GCN1 FW AGGREGATION*******\n";
      std::cout << "input row:" << dim.inputRows << "\n";
      std::cout << "input col:" << dim.inputColumns << "\n";
      std::cout << "output row:" << dim.outputRows << "\n";
      std::cout << "output col:" << dim.outputColumns << "\n";
      std::cout << "[[ FWAGGR INPUT ]] *****\n";
      for (std::uint64_t i = 0; i < in.size(); ++i) {
        std::cout << i << "," << in[i] << "\n";
      }
    }
#endif

    galois::doAll(
        Tpl{this->inColumnLen_, inputEmbeddings, gPtr}, aggrEmbeddings,
        +[](Tpl tpl, pando::Array<GNNFloat> outMat) {
          std::uint32_t host = pando::getCurrentPlace().node.id;

          pando::Array<bool> subgraph = fmap(*(tpl.gPtr), GetSubgraph, host);
          VertexDenseID rowLen = fmap(*(tpl.gPtr), GetSubgraphSize, host);
          LayerDimension columnLen = tpl.columnLen;
          pando::Array<GNNFloat> inMat = fmap(tpl.inMat, get, host);

          galois::doAll(
              InnerTpl{rowLen, columnLen, tpl.gPtr, subgraph, inMat, outMat},
              galois::IotaRange(0, rowLen), +[](InnerTpl tpl, VertexDenseID row) {
                std::uint32_t host = pando::getCurrentPlace().node.id;

                LayerDimension rowLen = tpl.rowLen;
                LayerDimension columnLen = tpl.columnLen;

                // Reset the output matrix
                for (LayerDimension f = 0; f < columnLen; ++f) {
                  tpl.outMat[row * columnLen + f] = 0;
                }

                // Aggregate adjacent vertex embeddings
                // Note that the dimension of the adjacent matrix is (rowLen x rowLen)
                for (LayerDimension ri = 0; ri < rowLen; ++ri) {
                  if (tpl.subgraph[row * rowLen + ri]) {
                    for (LayerDimension fi = 0; fi < columnLen; ++fi) {
                      tpl.outMat[row * columnLen + fi] += tpl.inMat[ri * columnLen + fi];
                    }
                  }
                }
              });
        });

#if 0
    std::cout << "GCN1 FORWARD output\n";
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      pando::GlobalRef<pando::Array<GNNFloat>> outRef = fmap(aggrEmbeddings, get, host);
      pando::Array<GNNFloat> out = outRef;
      for (std::uint64_t i = 0; i < lift(outRef, size); ++i) {
        std::cout << i << ":" << out[i] << "\n";
      }
    }
#endif
  }

  /**
   * @brief Update vertex embeddings by multiplying the current vertex embeddings times
   * this layer's weight matrix.
   */
  void UpdateEmbedding(galois::PerHost<pando::Array<GNNFloat>>& inputEmbeddings,
                       galois::PerHost<pando::Array<GNNFloat>>& outputMatrix) {
    gnn::multiplyMatricesPerHost(inputEmbeddings, this->layerWeights_, outputMatrix,
                                 this->dimensions_);

    // Output matrix <- Input matrix x Weight matrix
    // (input row x output column) <-
    // (input row x input column) x (input column x output column)
#if 0
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      pando::Array<GNNFloat> in = fmap(inputEmbeddings, get, host);
      pando::Array<GNNFloat> wm = fmap(this->layerWeights_, get, host);
      pando::Array<GNNFloat> out = fmap(outputMatrix, get, host);
      GNNLayerDimensions dim = fmap(this->dimensions_, get, host);
      std::cout << "UPDATE EMBEDDING*******\n";
      std::cout << "input row:" << dim.inputRows << "\n";
      std::cout << "input col:" << dim.inputColumns << "\n";
      std::cout << "output row:" << dim.outputRows << "\n";
      std::cout << "output col:" << dim.outputColumns << "\n";
      std::cout << "[[ INPUT ]] *****\n";
      for (std::uint64_t i = 0; i < in.size(); ++i) {
        std::cout << i << "," << in[i] << "\n";
      }

      std::cout << "[[ WEIGHT ]] *****\n";
      for (std::uint64_t i = 0; i < wm.size(); ++i) {
        std::cout << i << "," << wm[i] << "\n";
      }

      std::cout << "[[ OUTPUT ]] *****\n";
      for (std::uint64_t i = 0; i < out.size(); ++i) {
        std::cout << i << "," << out[i] << "\n";
      }
    }
#endif
  }

  /**
   * @brief This calculates weight gradient.
   * So, transposed input embeddings (input column x input row) x gradient (input row x output
   * column)
   */
  void CalculateWeightGradient(galois::PerHost<pando::Array<GNNFloat>>& vertexEmbedding,
                               galois::PerHost<pando::Array<GNNFloat>>& inputGradients,
                               galois::PerHost<pando::Array<GNNFloat>>& outputMatrix) {
    struct Tpl {
      galois::PerHost<GNNLayerDimensions> dim;
      galois::PerHost<pando::Array<GNNFloat>> inMat;
      galois::PerHost<pando::Array<GNNFloat>> inGradMat;
    };

    struct InnerTpl {
      GNNLayerDimensions dim;
      pando::Array<GNNFloat> outMat;
      pando::Array<GNNFloat> inMat;
      pando::Array<GNNFloat> inGradMat;
    };

    galois::doAll(
        Tpl{this->dimensions_, vertexEmbedding, inputGradients}, outputMatrix,
        +[](Tpl tpl, pando::Array<GNNFloat> outMat) {
          std::uint32_t host = pando::getCurrentPlace().node.id;

          GNNLayerDimensions dim = fmap(tpl.dim, get, host);
          pando::Array<GNNFloat> inMat = fmap(tpl.inMat, get, host);
          pando::Array<GNNFloat> inGradMat = fmap(tpl.inGradMat, get, host);

          // Reset an out matrix
          galois::doAll(
              outMat, +[](pando::GlobalRef<GNNFloat> v) {
                v = 0;
              });

          galois::doAll(
              InnerTpl{dim, outMat, inMat, inGradMat}, galois::IotaRange(0, dim.inputColumns),
              +[](InnerTpl tpl, LayerDimension c) {
                for (LayerDimension z = 0; z < tpl.dim.outputColumns; ++z) {
                  for (LayerDimension x = 0; x < tpl.dim.outputRows; ++x) {
                    tpl.outMat[c * tpl.dim.outputColumns + z] +=
                        tpl.inMat[x * tpl.dim.inputColumns + c] *
                        tpl.inGradMat[x * tpl.dim.outputColumns + z];
                  }
                }
              });
        });
  }

  /**
   * @brief This calculates layer embedding gradient.
   * So, gradient (input row x output column) x transposed weight (output column x input columns)
   */
  void CalculateLayerGradient(galois::PerHost<pando::Array<GNNFloat>>& inputGradients,
                              galois::PerHost<pando::Array<GNNFloat>>& outputMatrix) {
    struct Tpl {
      galois::PerHost<GNNLayerDimensions> dim;
      galois::PerHost<pando::Array<GNNFloat>> inGradMat;
      galois::PerHost<pando::Array<GNNFloat>> weightMat;
    };

    struct InnerTpl {
      GNNLayerDimensions dim;
      pando::Array<GNNFloat> outMat;
      pando::Array<GNNFloat> inGradMat;
      pando::Array<GNNFloat> weightMat;
    };

    galois::doAll(
        Tpl{this->dimensions_, inputGradients, this->layerWeights_}, outputMatrix,
        +[](Tpl tpl, pando::Array<GNNFloat> outMat) {
          std::uint32_t host = pando::getCurrentPlace().node.id;

          galois::doAll(
              outMat, +[](pando::GlobalRef<GNNFloat> v) {
                v = 0;
              });

          GNNLayerDimensions dim = fmap(tpl.dim, get, host);
          pando::Array<GNNFloat> inGradMat = fmap(tpl.inGradMat, get, host);
          pando::Array<GNNFloat> weightMat = fmap(tpl.weightMat, get, host);

          galois::doAll(
              InnerTpl{dim, outMat, inGradMat, weightMat}, galois::IotaRange(0, dim.inputRows),
              +[](InnerTpl tpl, LayerDimension r) {
                for (LayerDimension z = 0; z < tpl.dim.inputColumns; ++z) {
                  for (LayerDimension x = 0; x < tpl.dim.outputColumns; ++x) {
                    tpl.outMat[r * tpl.dim.inputColumns + z] +=
                        tpl.inGradMat[r * tpl.dim.outputColumns + x] *
                        tpl.weightMat[z * tpl.dim.outputColumns + x];
                  }
                }
              });
        });
  }

private:
  LayerDimension inColumnLen_;
  galois::PerHost<pando::Array<GNNFloat>> tempInputMatrix1_;
  galois::PerHost<pando::Array<GNNFloat>> tempInputMatrix2_;
  galois::PerHost<pando::Array<GNNFloat>> tempOutputMatrix_;
};

} // namespace gnn

#endif // PANDO_WF1_LAYERS_GCN_HPP_
