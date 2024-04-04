// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF1_GALOIS_GRAPHS_GNNGRAPH_HPP_
#define PANDO_WF1_GALOIS_GRAPHS_GNNGRAPH_HPP_

#include <algorithm>
#include <limits>
#include <utility>

#include <pando-rt/containers/vector.hpp>
#include <pando-rt/sync/mutex.hpp>

#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/graphs/dist_local_csr.hpp>
#include <pando-lib-galois/graphs/local_csr.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/utility/counted_iterator.hpp>
#include <pando-lib-galois/utility/dist_accumulator.hpp>

#include <pando-wf1-galois/gnntypes.hpp>
#include <pando-wf1-galois/minibatcher.hpp>

/**
 * @file gnngraph.hpp
 * TODO(hc)
 */

namespace gnn {

namespace graph {

/**
 * @brief Wrapper class of a graph for GNN.
 *
 * This class extends a plain graph type to support vertex type between training, testing, and
 * validation, graph sampling (ego graph construction), and a vertex/edge embedding.
 *
 */
template <typename InnerGraph>
class GNNGraph {
public:
  using VertexTopologyID = typename InnerGraph::VertexTopologyID;
  using EdgeHandle = typename InnerGraph::EdgeHandle;
  using VertexData = typename InnerGraph::VertexData;
  using EdgeData = typename InnerGraph::EdgeData;
  using VertexRange = typename InnerGraph::VertexRange;
  using VertexIterator = typename InnerGraph::VertexIt;
  using LCSR = typename InnerGraph::CSR;

  constexpr GNNGraph() noexcept = default;
  constexpr GNNGraph(GNNGraph<InnerGraph>&&) noexcept = default;
  constexpr GNNGraph(const GNNGraph<InnerGraph>&) noexcept = default;
  constexpr GNNGraph<InnerGraph>& operator=(const GNNGraph<InnerGraph>&) noexcept = default;
  constexpr GNNGraph<InnerGraph>& operator=(GNNGraph<InnerGraph>&&) noexcept = default;
  ~GNNGraph() = default;

  /**
   * @brief Initialize vertex/edge types, vertex/edge labels, vertex/edge features, and other
   * metadata for GNN phases.
   */
  void initialize(pando::GlobalPtr<InnerGraph> dGraphPtr, LayerDimension vertexFeatureLength = 30,
                  LayerDimension numClasses = 5) {
    std::cout << "[GNNGraph] Starts initialization\n" << std::flush;
    this->numClasses_ = numClasses;
    // TODO(hc): replace it with the GlobalPtr type
    this->dGraph_ = *dGraphPtr;
    this->vertexFeatureLength_ = vertexFeatureLength;
    PANDO_CHECK(this->vertexFeatures_.initialize());
    this->AllocateVertexEmbedding();
    // Construct histogram-based vertex embeddings with 1- and 2-hop neighbor vertices
    this->ConstructFeatureBy2HopAggregation();
    this->AllocateVertexTypeArrays();
    // Select training, test, and validation vertices
    this->InitializeVertexTypes();
    // Initialize normalized factors
    this->InitializeNormFactor();
    // Setup minibatching generator
    std::uint64_t batchSize =
        std::min(std::uint64_t{128}, this->numTrainingVertices_ /
                                         static_cast<std::uint64_t>(pando::getPlaceDims().node.id));
    this->trainMinibatcher_ = static_cast<decltype(this->trainMinibatcher_)>(
        pando::getDefaultMainMemoryResource()->allocate(sizeof(MinibatchGenerator<InnerGraph>)));
    this->trainMinibatcher_->initialize(this->trainingVertices_, this->batchVertices_, batchSize,
                                        this->dGraph_);
    this->testMinibatcher_ = static_cast<decltype(this->testMinibatcher_)>(
        pando::getDefaultMainMemoryResource()->allocate(sizeof(MinibatchGenerator<InnerGraph>)));
    this->testMinibatcher_->initialize(this->testVertices_, this->batchVertices_, batchSize,
                                       this->dGraph_);
    this->InitializePerHostGraphSampling();
    // Pointer pointing to a subgraph
    // this->subgraphPtr_ = nullptr;
    // This flag is set when a subgraph is ready
    this->useSubgraph_ = false;
    std::cout << "[GNNGraph] Starts initialization [DONE]\n" << std::flush;
  }

  /**
   * @brief Allocate vertex embedding arrays.
   *
   * @details Each vertex embedding is associated with a graph type.
   * This method iterates each vertex and allocates its embedding vector.
   * The vector is materialized on the PXN where owns it.
   */
  // TODO(hc): parallelize initialization
  void AllocateVertexEmbedding() {
    for (VertexTopologyID v : lift(this->dGraph_, vertices)) {
      pando::GlobalRef<VertexData> vDataRef = fmap(this->dGraph_, getData, v);
      VertexData vData = vDataRef;
      pando::Place place = fmap(this->dGraph_, getLocalityVertex, v);
      PANDO_CHECK(
          vData.embedding.initialize(this->vertexFeatureLength_, place, pando::MemoryType::Main));
      vDataRef = vData;
    }

    //     for (VertexTopologyID v : lift(this->dGraph_, vertices)) {
    //       std::uint64_t vid = this->dGraph_.getVertexIndex(v);
    //       pando::GlobalRef<VertexData> vDataRef = fmap(this->dGraph_, getData, v);
    //       VertexData vData = vDataRef;
    //       for (size_t i = 0; i < this->vertexFeatureLength_; ++i) {
    //         std::cout << vid << "," << i << "," << vData.embedding[i] << "\n";
    //       }
    //       vDataRef = vData;
    //     }
  }

  /**
   * @brief Construct features from 2-hop neighbors.
   *
   * @details This method consists of two steps:
   * 1) each vertex aggregates types of adjacent vertices and edges.
   * Then, accumulates the aggregated types to the first half of the vertex's feature.
   * 2) each vertex aggregates the first half of adjacent vertices' features, and
   * accumulates them to the second half of the vertex's feature.
   */
  void ConstructFeatureBy2HopAggregation() {
    std::cout << "[GNNGraph] Starts vertex feature construction\n" << std::flush;
    // Initialize all feature values to 0
    galois::doAll(
        this->dGraph_, this->dGraph_.vertices(), +[](InnerGraph innerGraph, VertexTopologyID v) {
          pando::GlobalRef<VertexData> vDataRef = fmap(innerGraph, getData, v);
          VertexData vData = vDataRef;
          for (size_t i = 0; i < vData.embedding.size(); ++i) {
            vData.embedding[i] = 0;
          }
        });

    // Accumulates types of adjacent vertices and edges, and constructs
    // the first half feature of the vertex.
    galois::doAll(
        this->dGraph_, this->dGraph_.vertices(), +[](InnerGraph g, VertexTopologyID v) {
          VertexData srcData = g.getData(v);
          pando::Array<GNNFloat> srcEmbed = srcData.embedding;
          for (EdgeHandle eh : g.edges(v)) {
            VertexTopologyID dst = g.getEdgeDst(eh);
            VertexData dstData = g.getData(dst);
            EdgeData edgeData = g.getEdgeData(eh);
            srcEmbed[static_cast<std::uint64_t>(dstData.type)] += 1;
            srcEmbed[static_cast<std::uint64_t>(edgeData.type)] += 1;
          }
        });

    // Accumulates the first half feature of adjacent vertices and edges,
    // and constructs the last half feature of the vertex.
    galois::doAll(
        this->dGraph_, this->dGraph_.vertices(), +[](InnerGraph g, VertexTopologyID v) {
          VertexData srcData = g.getData(v);
          pando::Array<GNNFloat> srcEmbed = srcData.embedding;
          std::uint64_t fLen = srcEmbed.size();
          std::uint64_t srcOffset = fLen / 2;
          for (EdgeHandle eh : g.edges(v)) {
            VertexTopologyID dst = g.getEdgeDst(eh);
            VertexData dstData = g.getData(dst);
            pando::Array<GNNFloat> dstEmbed = dstData.embedding;
            for (std::uint64_t f = 0; f < fLen / 2; ++f) {
              srcEmbed[srcOffset + f] += dstEmbed[f];
            }
          }
        });

    std::cout << "[GNNGraph] Completes vertex feature construction\n" << std::flush;
    /*
    std::uint64_t sumFeature{0};
    for (VertexTopologyID v : lift(this->dGraph_, vertices)) {
      std::uint64_t vid = this->dGraph_.getVertexIndex(v);
      pando::GlobalRef<VertexData> vDataRef = fmap(this->dGraph_, getData, v);
      VertexData vData = vDataRef;
      for (size_t i = 0; i < this->vertexFeatureLength_; ++i) {
        sumFeature += vData.embedding[i];
        std::cout << vid << "," << i << "," << vData.embedding[i] << "\n" << std::flush;
      }
    }
    std::cout << "--> " << sumFeature << "\n";
    */
  }

  /**
   * @brief Allocate and initialize vertex type arrays.
   *
   * @details Each vertex is used during either training, testing, validation, or minibatching.
   * This method allocates arrays to specify the vertex types.
   * For example, if a vertex i is chosen to a training vertex, trainingVertices_[i] is set to true.
   * These arrays are per-host arrays and each element is assigned to the PXN where the
   * corresponding vertex exists.
   */
  void AllocateVertexTypeArrays() {
    std::cout << "[GNNGraph] Vertex type array allocations\n" << std::flush;
    PANDO_CHECK(this->trainingVertices_.initialize());
    PANDO_CHECK(this->testVertices_.initialize());
    PANDO_CHECK(this->validationVertices_.initialize());
    PANDO_CHECK(this->batchVertices_.initialize());
    galois::doAll(
        this->dGraph_, this->trainingVertices_,
        +[](InnerGraph g, pando::GlobalRef<pando::Array<bool>> vsRef) {
          pando::Array<bool> vs = vsRef;
          pando::Place place = pando::getCurrentPlace();
          PANDO_CHECK(vs.initialize(g.localSize(place.node.id), place, pando::MemoryType::Main));
          galois::doAll(
              vs, +[](pando::GlobalRef<bool> v) {
                v = false;
              });
          vsRef = vs;
        });
    galois::doAll(
        this->dGraph_, this->testVertices_,
        +[](InnerGraph g, pando::GlobalRef<pando::Array<bool>> vsRef) {
          pando::Array<bool> vs = vsRef;
          pando::Place place = pando::getCurrentPlace();
          PANDO_CHECK(vs.initialize(g.localSize(place.node.id), place, pando::MemoryType::Main));
          galois::doAll(
              vs, +[](pando::GlobalRef<bool> v) {
                v = false;
              });
          vsRef = vs;
        });
    galois::doAll(
        this->dGraph_, this->validationVertices_,
        +[](InnerGraph g, pando::GlobalRef<pando::Array<bool>> vsRef) {
          pando::Array<bool> vs = vsRef;
          pando::Place place = pando::getCurrentPlace();
          PANDO_CHECK(vs.initialize(g.localSize(place.node.id), place, pando::MemoryType::Main));
          galois::doAll(
              vs, +[](pando::GlobalRef<bool> v) {
                v = false;
              });
          vsRef = vs;
        });
    galois::doAll(
        this->dGraph_, this->batchVertices_,
        +[](InnerGraph g, pando::GlobalRef<pando::Array<bool>> vsRef) {
          pando::Array<bool> vs = vsRef;
          pando::Place place = pando::getCurrentPlace();
          PANDO_CHECK(vs.initialize(g.localSize(place.node.id), place, pando::MemoryType::Main));
          galois::doAll(
              vs, +[](pando::GlobalRef<bool> v) {
                v = false;
              });
          vsRef = vs;
        });
    std::cout << "[GNNGraph] Vertex type array allocations [DONE]\n" << std::flush;
  }

  /**
   * @brief Calculate the ranges of each vertex type, and mark chosen vertex ids
   * for each vertex type in a boolean marker: TODO(hc): replace this with bitset later.
   */
  void InitializeVertexTypes() {
    // Decide the number of vertices for each type
    // These numbers are based on the AGILE's configuration
    this->numTrainingVertices_ = this->dGraph_.size() / 4;
    this->numTestingVertices_ = this->numTrainingVertices_ / 2;
    this->numValidatingVertices_ = this->numTestingVertices_;

    std::cout << "[GNNGraph] Num. training vertices: " << this->numTrainingVertices_ << ","
              << " Num. testing vertices: " << this->numTestingVertices_ << ","
              << " Num. validation vertices: " << this->numValidatingVertices_ << "\n";
    this->trainingVertexRange_ = {0, this->numTrainingVertices_, this->numTrainingVertices_};
    this->testVertexRange_ = {this->numTrainingVertices_,
                              this->numTrainingVertices_ + this->numTestingVertices_,
                              this->numTestingVertices_};
    this->validationVertexRange_ = {this->numTrainingVertices_ + this->numTestingVertices_,
                                    this->numTrainingVertices_ + 2 * this->numTestingVertices_,
                                    this->numTestingVertices_};
    // Sample vertices randomly
    this->RandomMaskSampling(this->trainingVertexRange_.end - this->trainingVertexRange_.begin,
                             this->trainingVertices_);
    this->RandomMaskSampling(this->testVertexRange_.end - this->testVertexRange_.begin,
                             this->testVertices_);
    this->RandomMaskSampling(this->validationVertexRange_.end - this->validationVertexRange_.begin,
                             this->validationVertices_);
  }

  /**
   * @brief Sample vertices for each type randomly.
   *
   * @detail A strategy that this method uses to sample vertices is to fill all vertex local dense
   * IDs to a local array, shuffle it, and then chooses vertices with IDs from
   * array[0] to array[# of a vertex type] as the type.
   */
  void RandomMaskSampling(std::uint64_t sampleSize, galois::PerHost<pando::Array<bool>>& mask) {
    galois::PerHost<pando::Array<std::uint64_t>> allVertices;
    PANDO_CHECK(allVertices.initialize());

    // Fills vertice local IDs to each per-host arrays.
    galois::doAll(
        this->dGraph_, allVertices,
        +[](InnerGraph g, pando::GlobalRef<pando::Array<std::uint64_t>> avRef) {
          std::uint32_t host = pando::getCurrentPlace().node.id;
          std::uint64_t numLocalVertices = g.localSize(host);

          pando::Array<std::uint64_t> av = avRef;
          PANDO_CHECK(av.initialize(numLocalVertices));

          for (std::uint64_t i = 0; i < av.size(); ++i) {
            av[i] = i;
          }

          std::uint32_t seed = time(nullptr);
          std::mt19937 tempDistr(rand_r(&seed));
          for (std::uint64_t i = av.size() - 1; i > 0; --i) {
            std::uniform_int_distribution<std::uint64_t> distr(0, i);
            std::uint64_t j = distr(tempDistr);
            std::uint64_t temp = av[i];
            av[i] = av[j];
            av[j] = temp;
          }
          avRef = av;
        });

    struct TplOut {
      std::uint64_t ss;
      galois::PerHost<pando::Array<bool>> masks;
      galois::PerHost<pando::Array<std::uint64_t>> avs;
    };

    struct TplIn {
      std::uint64_t ss;
      pando::Array<bool> mask;
      pando::Array<std::uint64_t> av;
    };

    // Sample each type of vertices per PXN
    galois::doAll(
        TplOut{sampleSize, mask}, allVertices,
        +[](TplOut tpl, pando::GlobalRef<pando::Array<std::uint64_t>> avRef) {
          std::uint64_t ss = tpl.ss;
          pando::Array<std::uint64_t> av = avRef;

          std::uint32_t host = pando::getCurrentPlace().node.id;
          std::uint32_t numHosts = pando::getPlaceDims().node.id;
          std::uint32_t numLocalSample = ss / numHosts;
          numLocalSample += (ss % numHosts > host) ? 1 : 0;

          pando::Array<bool> mask = fmap(tpl.masks, get, host);

          galois::doAll(
              TplIn{numLocalSample, mask, av}, galois::IotaRange(0, numLocalSample),
              +[](TplIn tpl, std::uint64_t i) {
                if (i >= tpl.ss) {
                  return;
                }
                tpl.mask[tpl.av[i]] = true;
              });
        });
    allVertices.deinitialize();

#if false
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      pando::GlobalRef<pando::Array<bool>> maskRef = fmap(mask, get, host);
      pando::Array<bool> mask = maskRef;
      for (std::uint64_t i = 0; i < mask.size(); ++i) {
        std::cout << "host:" << host << ", i:" << i << ", item:" << mask[i] << "\n";
      }
    }
#endif
  }

  /**
   * @brief Store outdegree for each vertex for normalization
   */
  void InitializeNormFactor() {
    PANDO_CHECK(this->vertexDegree_.initialize());

    struct Tpl {
      InnerGraph g;
      pando::Array<EdgeDenseID> vd;
    };

    galois::doAll(
        this->dGraph_, this->vertexDegree_,
        +[](InnerGraph g, pando::GlobalRef<pando::Array<EdgeDenseID>> vdRef) {
          std::uint32_t host = pando::getCurrentPlace().node.id;
          VertexDenseID numLocalVertices = g.localSize(host);
          PANDO_CHECK(fmap(vdRef, initialize, numLocalVertices));
          pando::Array<EdgeDenseID> vd = vdRef;

          pando::GlobalRef<LCSR> lcsr = lift(g, getLocalCSR);

          galois::doAll(
              Tpl{g, vd}, lift(lcsr, vertices), +[](Tpl tpl, VertexTopologyID v) {
                InnerGraph g = tpl.g;
                pando::Array<EdgeDenseID> vd = tpl.vd;
                VertexDenseID vid = fmap(g, getVertexLocalIndex, v);
                vd[vid] = fmap(g, getNumEdges, v);
              });
        });

#if false
    std::uint64_t totalDegree{0};
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      pando::GlobalRef<pando::Array<std::uint64_t>> vdRef = fmap(this->vertexDegree_, get, host);
      pando::Array<std::uint64_t> vd = vdRef;
      std::cout << "Host size:" << vd.size() << "\n";
      for (std::uint64_t i = 0; i < vd.size(); ++i) {
        std::cout << "host:" << host << ", i:" << i << ", degree:" << vd[i] << "\n";
        totalDegree += vd[i];
      }
    }
    std::cout << "Total degree:" << totalDegree << "\n";
#endif
  }

  /**
   * @brief Returns vertex type masks; for example, if the current phase is `GNNPhase::kTran`, it
   * returns a per-host array that marks vertices sampled for training.
   */
  galois::PerHost<pando::Array<bool>> GetVertexTypeMask(const GNNPhase currentPhase) {
    switch (currentPhase) {
      case GNNPhase::kTrain:
        return this->trainingVertices_;
      case GNNPhase::kTest:
        return this->testVertices_;
      case GNNPhase::kValidate:
        return this->validationVertices_;
      case GNNPhase::kBatch:
        return this->batchVertices_;
      default:
        std::cerr << "[GNNGraph] Failed to find a requested vertex mask\n" << std::flush;
        pando::exit(EXIT_FAILURE);
    }
  }
  pando::Array<bool> GetVertexTypeMask(const GNNPhase currentPhase, std::uint32_t host) {
    switch (currentPhase) {
      case GNNPhase::kTrain:
        return fmap(this->trainingVertices_, get, host);
      case GNNPhase::kTest:
        return fmap(this->testVertices_, get, host);
      case GNNPhase::kValidate:
        return fmap(this->validationVertices_, get, host);
      case GNNPhase::kBatch:
        return fmap(this->batchVertices_, get, host);
      default:
        std::cerr << "[GNNGraph] Failed to find a requested vertex mask\n" << std::flush;
        pando::exit(EXIT_FAILURE);
    }
  }

  /**
   * @brief Get the total number of vertex classes.
   */
  std::uint64_t GetNumClasses() {
    return static_cast<std::uint64_t>(this->numClasses_);
  }

  /**
   * @brief Reset minibatcher state for the next epoch.
   */
  void ResetTrainMinibatch() {
    this->trainMinibatcher_->ResetMinibatching();
  }

  /**
   * @brief Prepare and get the next minibatch.
   */
  VertexDenseID PrepareNextTrainMinibatch() {
    this->trainMinibatcher_->GetNextMinibatch();
    return this->SampleSeedVertices(GNNPhase::kBatch);
  }

  /**
   * @brief Return true if all vertices have used for minibatching.
   */
  bool NoMoreTrainMinibatching() {
    return this->trainMinibatcher_->NoMoreMinibatching();
  }

  /**
   * @brief Reset test minibatcher state for the next epoch.
   */
  void ResetTestMinibatch() {
    this->testMinibatcher_->ResetMinibatching();
  }

  /**
   * @brief Prepare and get the next test minibatch.
   */
  std::uint64_t PrepareNextTestMinibatch() {
    this->testMinibatcher_->GetNextMinibatch();
    return this->SampleSeedVertices(GNNPhase::kBatch);
  }

  /**
   * @brief Return true if all vertices have used for minibatching.
   */
  bool NoMoreTestMinibatching() {
    return this->testMinibatcher_->NoMoreMinibatching();
  }

  /**
   * @brief Initialize per-host objects.
   *
   * @note This method should be called once.
   */
  void InitializePerHostGraphSampling() {
    PANDO_CHECK(this->sampledVertices_.initialize());
    PANDO_CHECK(this->subgraph_.initialize());
    PANDO_CHECK(this->sampledSrcs_.initialize());
    PANDO_CHECK(this->sampledDsts_.initialize());
    PANDO_CHECK(this->numSampledVertices_.initialize());
    PANDO_CHECK(this->subgraphIdMapping_.initialize());

    struct Tpl {
      InnerGraph g;
      galois::PerHost<pando::Vector<VertexDenseID>> idMapping;
    };

    galois::doAll(
        Tpl{this->dGraph_, this->subgraphIdMapping_}, this->sampledVertices_,
        +[](Tpl tpl, pando::GlobalRef<pando::Array<bool>> svRef) {
          std::uint32_t host = pando::getCurrentPlace().node.id;
          VertexDenseID numLocalVertices = fmap(tpl.g, localSize, host);
          PANDO_CHECK(fmap(svRef, initialize, numLocalVertices, pando::getCurrentPlace(),
                           pando::MemoryType::Main));

          // Each index is corresponding to subraph's local vertex ids
          // This vector is initialized once and is reused during epochs
          // FYI, its size is the number of original vertices to avoid reallocation.
          // Values indexed non-sampled vertex IDs are set to the index type's max value.
          pando::GlobalRef<pando::Vector<VertexDenseID>> idMapping = fmap(tpl.idMapping, get, host);
          PANDO_CHECK(fmap(idMapping, initialize, numLocalVertices));
        });
  }

  /**
   * @brief Reset states and objects for graph sampling.
   */
  void ResetSamplingState() {
    struct Tpl {
      InnerGraph g;
      bool useSubgraph;
      galois::PerHost<pando::Vector<VertexDenseID>> sampledSrcs;
      galois::PerHost<pando::Vector<VertexDenseID>> sampledDsts;
      galois::PerHost<VertexDenseID> numSampledVertices;
      galois::PerHost<pando::Vector<VertexDenseID>> idMapping;
    };

    galois::doAll(
        Tpl{this->dGraph_, this->useSubgraph_, this->sampledSrcs_, this->sampledDsts_,
            this->numSampledVertices_, this->subgraphIdMapping_},
        this->sampledVertices_, +[](Tpl tpl, pando::GlobalRef<pando::Array<bool>> svRef) {
          std::uint32_t host = pando::getCurrentPlace().node.id;
          pando::Array<bool> sv = svRef;
          bool useSubgraph = tpl.useSubgraph;
          pando::GlobalRef<pando::Vector<VertexDenseID>> sss = fmap(tpl.sampledSrcs, get, host);
          pando::GlobalRef<pando::Vector<VertexDenseID>> sds = fmap(tpl.sampledDsts, get, host);

          pando::GlobalRef<VertexDenseID> numSampledVertices =
              fmap(tpl.numSampledVertices, get, host);
          numSampledVertices = 0;
          galois::doAll(
              sv, +[](pando::GlobalRef<bool> v) {
                v = false;
              });

          if (useSubgraph) {
            liftVoid(sss, deinitialize);
            liftVoid(sds, deinitialize);
          }

          PANDO_CHECK(fmap(sss, initialize, 0));
          PANDO_CHECK(fmap(sds, initialize, 0));

          // Initialize subgraph local IDs
          LCSR g = lift(tpl.g, getLocalCSR);
          galois::doAll(
              g, g.vertices(), +[](LCSR g, VertexTopologyID v) {
                pando::GlobalRef<VertexData> vDataRef = fmap(g, getData, v);
                fmapVoid(vDataRef, SetSID, std::numeric_limits<VertexDenseID>::max());
              });

          /*
          for (VertexTopologyID v : fmap(g, vertices)) {
            VertexDenseID vid = fmap(g, getVertexIndex, v);
            VertexData vData = fmap(g, getData, v);
            std::cout << host << ":" << vid << "==" << vData.sid << "\n";
          }
          */

          // Initialize a mapping between an original graph to a subgraph to max
          pando::Vector<VertexDenseID> idMapping = fmap(tpl.idMapping, get, host);
          galois::doAll(
              idMapping, +[](pando::GlobalRef<VertexDenseID> v) {
                v = std::numeric_limits<VertexDenseID>::max();
              });
        });
  }

  /**
   * @brief Sample seed vertices.
   *
   * @details Graph is sampled and ego graphs are constructed during training, testing, and
   * validation phases. This method samples seed vertices that have originally chosen for each
   * phase. From these seed vertices, k-hop vertices are randomly sampled.
   *
   * @return The total number of sampled vertices across PXNs
   */
  VertexDenseID SampleSeedVertices(GNNPhase seedPhase) {
    // If this is for an epoch > 0, a subgraph should be enabled, and graph sampling is performed
    // based on the subgraph. This should sample from the original graph.
    this->DisableSubgraph();

    struct Tpl {
      InnerGraph g;
      galois::PerHost<pando::Array<bool>> sampled;
      galois::PerHost<pando::Array<bool>> mask;
      galois::DAccumulator<VertexDenseID> accum;
    };

    // Set seed vertices; These vertices are selected during the vertex type selection phase
    galois::DAccumulator<VertexDenseID> numSeedVertices;
    PANDO_CHECK(numSeedVertices.initialize());
    galois::doAll(
        Tpl{this->dGraph_, this->sampledVertices_, this->GetVertexTypeMask(seedPhase),
            numSeedVertices},
        this->dGraph_.vertices(), +[](Tpl tpl, VertexTopologyID v) {
          std::uint32_t host = pando::getCurrentPlace().node.id;

          pando::Array<bool> sampled = fmap(tpl.sampled, get, host);
          pando::Array<bool> mask = fmap(tpl.mask, get, host);
          galois::DAccumulator<VertexDenseID> accum = tpl.accum;
          VertexDenseID vid = fmap(tpl.g, getVertexLocalIndex, v);

          // Seed vertices are the vertices that were chosen for `seedPhase.`
          if (mask[vid]) {
            sampled[vid] = true;
            accum.increment();
          } else {
            sampled[vid] = false;
          }
        });

    /*
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      pando::Array<bool> sampled = fmap(this->sampledVertices_, get, host);
      for (VertexDenseID i = 0; i < sampled.size(); ++i) {
        std::cout << "host:" << host << ", i:" << i << ", is sampled:" << sampled[i] << "\n";
      }
    }
    */

    VertexDenseID totalSampledVertices = numSeedVertices.reduce();
    numSeedVertices.deinitialize();
    return totalSampledVertices;
  }

  /**
   * @brief Samples outgoing edges and vertices from seed vertices.
   * Different from `SampleEdges()`, all layers will use the same sampled graph.
   */
  galois::PerHost<VertexDenseID> SampleEdges() {
    std::cout << "[GNNGraph] Starts graph sampling\n" << std::flush;
    // TODO(hc): This is parallelized only in PXN level.
    // AGILE WF1 VC asynchronously samples edges per each vertex in parallel.
    // This version needs to increase parallelism to the vertex level.

    galois::PerHost<pando::Vector<VertexDenseID>> frontier;
    PANDO_CHECK(frontier.initialize());

    struct Tpl {
      InnerGraph g;
      galois::PerHost<pando::Array<bool>> svs;
      galois::DAccumulator<VertexDenseID> accum;
      galois::PerHost<pando::Array<bool>> subgraph;
      galois::PerHost<pando::Vector<VertexDenseID>> idMapping;
      galois::PerHost<pando::Vector<VertexDenseID>> sampledSrcs;
      galois::PerHost<pando::Vector<VertexDenseID>> sampledDsts;
      galois::PerHost<VertexDenseID> numSampledVertices;
      bool useSubgraph;
    };

    galois::DAccumulator<VertexDenseID> totalVertexAccum;
    PANDO_CHECK(totalVertexAccum.initialize());

    galois::doAll(
        Tpl{this->dGraph_, this->sampledVertices_, totalVertexAccum, this->subgraph_,
            this->subgraphIdMapping_, this->sampledSrcs_, this->sampledDsts_,
            this->numSampledVertices_, this->useSubgraph_},
        frontier, +[](Tpl tpl, pando::Vector<VertexDenseID> frontier) {
          pando::Place currPlace = pando::getCurrentPlace();
          std::uint32_t host = currPlace.node.id;

          // NOTE: AGILE WF1 VC has a list of the number of edges to be sampled for each hop,
          // and its size is 5. The first element of this list (So, it is supposed to be
          // the number of adjacent (1-hop) edges to be sampled) is 5. But it is not used
          // and 3 of adjacent vertices are sampled.
          std::uint32_t numLevels{4};
          pando::Array<EdgeDenseID> levels;
          // TODO(hc): Can it be L2SP?
          PANDO_CHECK(levels.initialize(numLevels, currPlace, pando::MemoryType::Main));
          levels[0] = 3;
          levels[1] = 2;
          levels[2] = 1;
          levels[3] = 0;

          // Get a local sampled vertex array
          pando::Array<bool> sv = fmap(tpl.svs, get, host);

          pando::GlobalRef<LCSR> localGraph = lift(tpl.g, getLocalCSR);
          EdgeDenseID numSampledEdges{0}, numSampledVertices{0};

          // Sampled source/destination vertices
          pando::GlobalRef<pando::Vector<VertexDenseID>> sss = fmap(tpl.sampledSrcs, get, host);
          pando::GlobalRef<pando::Vector<VertexDenseID>> sds = fmap(tpl.sampledDsts, get, host);

          // Subgraph local ID
          VertexDenseID sid{0};
          // Mapping between original graph to subgraph
          pando::Vector<VertexDenseID> mapping = fmap(tpl.idMapping, get, host);

          PANDO_CHECK(frontier.initialize(0));
          // Push seed vertices to the frontier vector
          for (VertexTopologyID v : lift(localGraph, vertices)) {
            VertexDenseID vid = fmap(localGraph, getVertexIndex, v);
            pando::GlobalRef<VertexData> vData = fmap(localGraph, getData, v);

            if (sv[vid]) {
              // Assign a subgraph vertex ID to an original graph vertex ID
              mapping[sid] = vid;
              fmapVoid(vData, SetSID, sid++);
              PANDO_CHECK(frontier.pushBack(vid));
              numSampledVertices += 1;
            }
          }

          std::uint32_t level{0};
          // Track the current and last index of the frontier
          VertexDenseID frontierIndex{0}, frontierLast{frontier.size()};

          // TODO(hc): This random generator explodes stack memory usage.
          // Later, this should be replaced.
          std::random_device rd;
          std::mt19937_64 rng(rd());

          while (level < levels.size()) {
            if (frontierIndex == frontierLast) {
              break;
            }

            VertexDenseID vid = frontier[frontierIndex++];
            VertexTopologyID v = fmap(localGraph, getTopologyIDFromIndex, vid);
            pando::GlobalRef<VertexData> srcData = fmap(localGraph, getData, v);
            VertexDenseID ssid = lift(srcData, GetSID);
            EdgeDenseID numEdges = fmap(localGraph, getNumEdges, v);
            bool notLastLevel = level < (levels.size() - 1);
            if (numEdges != 0 && (notLastLevel || sv[vid])) {
              // TODO(hc): If an edge destination vertex is a remote vertex, the current version
              // skips its sampling. This decreases the total number of sampled edges, and may
              // decrease training quality. Later, samples the remote vertices with atomic ops.
              EdgeDenseID numEdgesToFetch =
                  std::min(static_cast<EdgeDenseID>(levels[level]), numEdges);
              std::uniform_int_distribution<EdgeDenseID> dist(0, numEdges - 1);
              // Sample adjacent outgoing edges until the count becomes the
              // target number of edges
              for (EdgeDenseID i = 0; i < numEdgesToFetch; ++i) {
                EdgeHandle e = fmap(localGraph, mintEdgeHandle, v, dist(rng));
                VertexTopologyID dst = fmap(localGraph, getEdgeDst, e);
                // TODO(hc): Need to revisit this part; how to handle if
                // a destination vertex is remote?
                if (fmap(localGraph, getLocalityVertex, dst).node.id != host) {
                  // A destination vertex can be a remote vertex
                  numEdges -= 1;
                  // If the number of outgoing edges pointing to a local vertex is less than
                  // the number of edges to fetch, keeps sampling the original number of
                  // edges to fetch.
                  if (numEdgesToFetch - i + 1 <= numEdges) {
                    i -= 1;
                  }
                  continue;
                }
                pando::GlobalRef<VertexData> dstData = fmap(localGraph, getData, dst);
                VertexDenseID did = fmap(localGraph, getVertexIndex, dst);
                ++numSampledEdges;
                if (!sv[did]) {
                  sv[did] = true;
                  numSampledVertices += 1;
                  // Assign a subgraph vertex ID to an original graph vertex ID
                  mapping[sid] = did;
                  fmapVoid(dstData, SetSID, sid++);
                  // The last level does not add frontiers to finish sampling
                  if (notLastLevel) {
                    PANDO_CHECK(frontier.pushBack(did));
                  }
                }

                VertexDenseID dsid = lift(dstData, GetSID);
                // std::cout << host << " in From " << vid << "(" << ssid << ") sampled " << did <<
                // "("
                //           << dsid << ")\n";
                PANDO_CHECK(fmap(sss, pushBack, ssid));
                PANDO_CHECK(fmap(sds, pushBack, dsid));
                PANDO_CHECK(fmap(sss, pushBack, dsid));
                PANDO_CHECK(fmap(sds, pushBack, ssid));
              }
            }

            if (frontierIndex == frontierLast) {
              ++level;
              frontierLast = frontier.size();
            }
          } // Vertex/Edge sampling is completed

          std::cout << "[GNNGraph] PXN " << host << ": Num sampled edges:" << numSampledEdges
                    << "\n";
          std::cout << "[GNNGraph] PXN " << host << ": Num sampled vertices:" << numSampledVertices
                    << "\n";

          fmap(tpl.numSampledVertices, get, host) = numSampledVertices;

          // Accumulate the number of local sampled vertifces
          galois::DAccumulator<VertexDenseID> accum = tpl.accum;
          accum.increment();
        });

    /*
    std::cout << "Mapping Print\n";
    pando::GlobalRef<pando::Vector<VertexDenseID>> mappingRef =
        fmap(this->subgraphIdMapping_, get, 0);
    pando::Vector<VertexDenseID> mapping = mappingRef;
    for (size_t i = 0; i < mapping.size(); ++i) {
      std::cout << i << " vs " << mapping[i] << "\n";
    }
    */

    // Materialize an adjacent matrix for the sampled vertices and edges
    this->ConstructSubgraph();
    this->useSubgraph_ = true;

    std::cout << "Total sampled vertices across PXNs:" << totalVertexAccum.reduce() << "\n";
    std::cout << "[GNNGraph] Starts graph sampling [DONE]\n" << std::flush;

    return this->numSampledVertices_;
  }

  /**
   * @brief Materialize an adjacent matrix for the sampled vertices and edges.
   *
   * @details New local ids of the Sampled vertices and edges on the future subgraph
   * are aggregated to per-host vectors on `SampleEdges()`. Based on that,
   * this method creates an adjacent matrix.
   */
  void ConstructSubgraph() {
    struct Tpl {
      galois::PerHost<pando::Vector<VertexDenseID>> sampledSrcs;
      galois::PerHost<pando::Vector<VertexDenseID>> sampledDsts;
      bool useSubgraph;
      galois::PerHost<VertexDenseID> numSampledVertices;
    };

    // Construct a matrix corresponding to a subgraph
    galois::doAll(
        Tpl{this->sampledSrcs_, this->sampledDsts_, this->useSubgraph_, this->numSampledVertices_},
        this->subgraph_, +[](Tpl tpl, pando::GlobalRef<pando::Array<bool>> subgraphRef) {
          pando::Place currPlace = pando::getCurrentPlace();
          std::uint32_t host = currPlace.node.id;
          VertexDenseID numSampledVertices = fmap(tpl.numSampledVertices, get, host);
          VertexDenseID subgraphDim = numSampledVertices * numSampledVertices;

          // Sampled source/destination vertices
          pando::GlobalRef<pando::Vector<VertexDenseID>> sss = fmap(tpl.sampledSrcs, get, host);
          pando::GlobalRef<pando::Vector<VertexDenseID>> sds = fmap(tpl.sampledDsts, get, host);
          VertexDenseID numSampledEdges = lift(sss, size);

          pando::Array<bool> subgraph = subgraphRef;
          VertexDenseID currSubgraphSize = subgraph.size();
          if (tpl.useSubgraph && currSubgraphSize <= subgraphDim) {
            subgraph.deinitialize();
          }

          if ((tpl.useSubgraph && currSubgraphSize <= subgraphDim) || !tpl.useSubgraph) {
            PANDO_CHECK(subgraph.initialize(subgraphDim, currPlace, pando::MemoryType::Main));
            subgraphRef = subgraph;
          }

          // Initialize an adjacent matrix; this method can reuse the matrix
          // constructed on the past epochs
          galois::doAll(
              subgraph, +[](pando::GlobalRef<bool> v) {
                v = false;
              });

          // Fill the dense adjacent matrix with the sampled vertices and edges
          struct Tpl {
            VertexDenseID rowDim;
            pando::Vector<VertexDenseID> srcs;
            pando::Vector<VertexDenseID> dsts;
            pando::Array<bool> subgraph;
          };
          galois::doAll(
              Tpl{numSampledVertices, sss, sds, subgraph}, galois::IotaRange(0, numSampledEdges),
              +[](Tpl tpl, std::uint64_t i) {
                pando::Array<bool> subgraph = tpl.subgraph;
                pando::Vector<VertexDenseID> src = tpl.srcs;
                pando::Vector<VertexDenseID> dst = tpl.dsts;
                VertexDenseID sid = src[i];
                VertexDenseID did = dst[i];
                subgraph[sid * tpl.rowDim + did] = true;
              });
        });
  }

  /**
   * @brief If this flag is enabled, a subgraph is ready and graph access is redirected
   * to that.
   */
  void EnableSubgraph() noexcept {
    this->useSubgraph_ = true;
  }

  /**
   * @brief If this flag is disabled, a subgraph might not be ready and graph access is redirected
   * to the original graph.
   */
  void DisableSubgraph() noexcept {
    this->useSubgraph_ = false;
  }

  /**
   * @brief Get vertex data.
   */
  pando::GlobalRef<VertexData> GetData(VertexTopologyID v) {
    return this->dGraph_.getData(v);
  }

  /**
   * @brief Get subgraph adjacent matrix.
   */
  pando::Array<bool> GetSubgraph(std::uint32_t host) {
    return fmap(this->subgraph_, get, host);
  }

  /**
   * @brief Get the number of vertices of the local subgraph
   */
  VertexDenseID GetSubgraphSize(std::uint32_t host) {
    return fmap(this->numSampledVertices_, get, host);
  }

  /**
   * @brief Get a vertex local id of the original graph from a vertex local id
   * of the subgraph.
   */
  VertexDenseID GetVIdFromSubgraphVId(std::uint32_t host, VertexDenseID subgraphVId) {
    pando::GlobalRef<pando::Vector<VertexDenseID>> mappingRef =
        fmap(this->subgraphIdMapping_, get, host);
    return fmap(mappingRef, get, subgraphVId);
  }

  /**
   * @brief Get a vertex topology id from a vertex dense id.
   *
   * @note This method assumes that it is called from a PXN context where the
   * target local CSR is materialized.
   */
  VertexTopologyID GetTopologyIDFromIndex(VertexDenseID vId) {
    // XXX: This method should be called from a PXN where this graph is materialized
    pando::GlobalRef<LCSR> localGraphRef = lift(this->dGraph_, getLocalCSR);
    return fmap(localGraphRef, getTopologyIDFromIndex, vId);
  }

  /**
   * @brief Get the length of each vertex embedding.
   */
  LayerDimension VertexFeatureLength() {
    return this->vertexFeatureLength_;
  }

  /**
   * @brief Get the length of each vertex embedding.
   */
  void SetVertexFeatureLength(LayerDimension sz) {
    this->vertexFeatureLength_ = sz;
  }

  /**
   * @brief Get a vertex local id from a vertex topological id.
   */
  VertexDenseID GetVertexIndex(VertexTopologyID v) {
    return this->dGraph_.getVertexIndex(v);
  }

  /**
   * @brief Get the global vertex iterator.
   */
  VertexRange Vertices() {
    return this->dGraph_.vertices();
  }

  /**
   * @brief Calculate accuracy.
   */
  std::pair<VertexDenseID, VertexDenseID> GetGlobalAccuracy(
      galois::PerHost<pando::Array<GNNFloat>>& predictions, GNNPhase phase) {
    galois::DAccumulator<VertexDenseID> totalAccum, correctAccum;
    PANDO_CHECK(totalAccum.initialize());
    PANDO_CHECK(correctAccum.initialize());
    totalAccum.reset();
    correctAccum.reset();

    // doAll body should access PGAS locale objects.
    // Unpack and copy necessary data across PXNs.
    struct Tpl {
      GNNPhase phase;
      // TODO(hc): This is not necessarily the full gnn graph, but
      // can be separate arrays
      GNNGraph<InnerGraph> g;
      galois::DAccumulator<VertexDenseID> totalAccum;
      galois::DAccumulator<VertexDenseID> correctAccum;
    };

    struct InnerTpl {
      GNNGraph<InnerGraph> g;
      pando::Array<GNNFloat> predictions;
      pando::Array<bool> mask;
      galois::DAccumulator<VertexDenseID> totalAccum;
      galois::DAccumulator<VertexDenseID> correctAccum;
    };

    galois::doAll(
        Tpl{phase, *this, totalAccum, correctAccum}, predictions,
        +[](Tpl tpl, pando::Array<GNNFloat> predictions) {
          std::uint32_t host = pando::getCurrentPlace().node.id;

          pando::Array<bool> mask = fmap(tpl.g, GetVertexTypeMask, tpl.phase, host);
          galois::DAccumulator<VertexDenseID> correctAccum = tpl.correctAccum;
          galois::DAccumulator<VertexDenseID> totalAccum = tpl.totalAccum;

          VertexDenseID subgraphSize = fmap(tpl.g, GetSubgraphSize, host);

          galois::doAll(
              InnerTpl{tpl.g, predictions, mask, correctAccum, totalAccum},
              galois::IotaRange(0, subgraphSize), +[](InnerTpl tpl, VertexDenseID subVid) {
                std::uint32_t host = pando::getCurrentPlace().node.id;
                std::uint64_t numClasses = lift(tpl.g, GetNumClasses);

                VertexDenseID vid = fmap(tpl.g, GetVIdFromSubgraphVId, host, subVid);
                if (tpl.mask[vid]) {
                  // Groundtruth
                  VertexTopologyID v = fmap(tpl.g, GetTopologyIDFromIndex, vid);
                  VertexData vData = fmap(tpl.g, GetData, v);
                  VertexDenseID groundTruth = fmap(tpl.g, GetGroundTruth, vData.type);

                  VertexDenseID maxId{std::numeric_limits<VertexDenseID>::max()};
                  GNNFloat maxValue{-std::numeric_limits<GNNFloat>::max()};
                  for (LayerDimension i = 0; i < numClasses; ++i) {
                    GNNFloat prediction = tpl.predictions[subVid * numClasses + i];
                    if (prediction > maxValue) {
                      maxValue = prediction;
                      maxId = i;
                    }
                  }
                  if (maxId == groundTruth) {
                    tpl.correctAccum.increment();
                  }
                  tpl.totalAccum.increment();
                }
              });
        });

#if 0
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      for (VertexTopologyID v : this->Vertices()) {
        std::uint64_t vid = this->GetVertexIndex(v);
        VertexData vData = this->GetData(v);
        std::cout << vid << " type " << this->GetGroundTruth(vData.type) << "\n";
      }

      std::cout << "subgraph prediction\n";
      pando::Array<bool> subgraph = this->GetSubgraph(host);

      pando::Array<GNNFloat> pred = fmap(predictions, get, host);
      VertexDenseID subgraphSize = this->GetSubgraphSize(host);
      for (VertexDenseID subvid = 0; subvid < subgraphSize; ++subvid) {
        std::cout << subvid << "(" << this->GetVIdFromSubgraphVId(host, subvid) << ")\n";
        for (LayerDimension ri = 0; ri < this->numClasses_; ++ri) {
          std::cout << pred[subvid * this->numClasses_ + ri] << " " << "\n";
        }
        std::cout << "\n";
      }
    }
#endif

    std::uint64_t totalChecks = totalAccum.reduce();
    std::uint64_t corrects = correctAccum.reduce();
    return std::make_pair(corrects, totalChecks);
  }

  /**
   * @brief Return the number of local vertices.
   */
  VertexDenseID LocalSize(std::uint32_t host) {
    return this->dGraph_.localSize(host);
  }

  /**
   * @brief Convert Agile type to a vertex class.
   *
   * @details Offset of the vertex token ID is changed based on the
   * Agile object type, and so the current AGILE types in lib-gal-root
   * are mapped to the offset. However, WF1 VC requires 0-indexed
   * vertex type for each Agile type.
   * This method converts it.
   */
  VertexDenseID GetGroundTruth(agile::TYPES type) {
    if (type == agile::TYPES::PERSON) {
      return 0;
    } else if (type == agile::TYPES::FORUMEVENT) {
      return 1;
    } else if (type == agile::TYPES::FORUM) {
      return 2;
    } else if (type == agile::TYPES::PUBLICATION) {
      return 3;
    } else if (type == agile::TYPES::TOPIC) {
      return 4;
    } else {
      std::cerr << static_cast<VertexDenseID>(type) << " type does not exist.\n" << std::flush;
      pando::exit(EXIT_FAILURE);
    }
  }

private:
  /// @brief Underlying distributed CSR graph
  InnerGraph dGraph_;
  /// @brief Length of the Vertex embedding
  LayerDimension vertexFeatureLength_;
  /// @brief Vertex embeddings
  galois::PerHost<pando::Vector<GNNFloat>> vertexFeatures_;
  /// @brief Number of classes for vertexs/edges
  LayerDimension numClasses_;
  /// @brief Vertex type masks: training, testing, validation
  galois::PerHost<pando::Array<bool>> trainingVertices_;
  galois::PerHost<pando::Array<bool>> testVertices_;
  galois::PerHost<pando::Array<bool>> validationVertices_;
  galois::PerHost<pando::Array<bool>> batchVertices_;
  /// @brief Number of vertices for each type
  std::uint64_t numTrainingVertices_;
  std::uint64_t numTestingVertices_;
  std::uint64_t numValidatingVertices_;
  /// @brief Vertex ranges for each vertex types
  GNNRange trainingVertexRange_;
  GNNRange testVertexRange_;
  GNNRange validationVertexRange_;
  /// @brief Degree for each vertex
  galois::PerHost<pando::Array<EdgeDenseID>> vertexDegree_;
  /// @brief Minibatch generator
  pando::GlobalPtr<MinibatchGenerator<InnerGraph>> trainMinibatcher_;
  pando::GlobalPtr<MinibatchGenerator<InnerGraph>> testMinibatcher_;
  /// @brief Per-host sampled vertices
  galois::PerHost<pando::Array<bool>> sampledVertices_;
  /// @brief True if a subgraph has been constructed and is used
  bool useSubgraph_;
  galois::PerHost<pando::Array<bool>> subgraph_;
  /// @brief Per-host subgraph vertex ID mapping to original graph vertex ID
  galois::PerHost<pando::Vector<VertexDenseID>> subgraphIdMapping_;
  /// @brief Per-host sampled source and destination
  galois::PerHost<pando::Vector<VertexDenseID>> sampledSrcs_;
  galois::PerHost<pando::Vector<VertexDenseID>> sampledDsts_;
  /// @brief The number of sampled vertices for each host
  galois::PerHost<VertexDenseID> numSampledVertices_;
};

} // namespace graph

} // namespace gnn

#endif // PANDO_WF1_GALOIS_GRAPHS_GNNGRAPH_HPP_
