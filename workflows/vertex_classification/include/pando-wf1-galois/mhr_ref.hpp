// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF1_GALOIS_MHR_REF_HPP_
#define PANDO_WF1_GALOIS_MHR_REF_HPP_

#include <string>
#include <utility>

#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/containers/per_thread.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/sorts/merge_sort.hpp>
#include <pando-lib-galois/utility/pair.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-wf1-galois/graphs/mhr_graph.hpp>

void debugPrint(const std::string& msg, const std::string& file, int line,
                const std::string& func) {
  std::cout << "Debug: [" << file << ":" << line << " (" << func << ")] " << msg << std::endl;
}

#define DEBUG_PRINT(msg) debugPrint(msg, __FILE__, __LINE__, __func__)

namespace mhr_ref {

enum class TYPES { PERSON, UNIVERSITY, DEEPLEARNING };

template <typename Graph>
class MHR_REF {
  using VertexTopologyID = typename Graph::VertexTopologyID;
  using VertexTokenID = typename Graph::VertexTokenID;
  using ResultStruct = galois::Pair<float, VertexTokenID>;
  using VertexData = typename Graph::VertexData;

  static constexpr float gamma = 1.0;

  struct ScoreState {
    ScoreState() = default;
    ScoreState(Graph graph_, galois::PerThreadVector<ResultStruct> scores_,
               typename Graph::VertexData head_, wf1::RelationFeatures relationFeatures_,
               wf1::MHR_ENTITY entity_type_, uint64_t relation_type_)
        : graph(graph_),
          scores(scores_),
          head(head_),
          relationFeatures(relationFeatures_),
          entity_type(entity_type_),
          relation_type(relation_type_) {}

    Graph graph;
    galois::PerThreadVector<ResultStruct> scores;
    typename Graph::VertexData head;
    wf1::RelationFeatures relationFeatures;
    wf1::MHR_ENTITY entity_type;
    uint64_t relation_type;
  };

  static void computeScore(pando::Vector<float>& result, const pando::Vector<double>& head,
                           const pando::Vector<double>& relation,
                           const pando::Vector<double>& entity) {
    for (int i = 0; i < result.size(); i++) {
      result[i] = head[i] + relation[i] - entity[i];
    }
  }

  static float computeL1Norm(const pando::Vector<float>& vec) {
    float score = 0;
    for (float elt : vec) {
      if (elt < 0) {
        score -= elt;
      } else {
        score += elt;
      }
    }
    return gamma - score;
  }

public:
  pando::Vector<VertexTokenID> computeScores(Graph graph, wf1::RelationFeatures relationFeatures,
                                             wf1::MHR_ENTITY entity_type,
                                             std::uint64_t relation_type, VertexTokenID head_id) {
    VertexData head = graph.getData(graph.getTopologyID(head_id));
    galois::PerThreadVector<ResultStruct> scores;
    PANDO_CHECK(scores.initialize());

    galois::doAll(
        ScoreState(graph, scores, head, relationFeatures, entity_type, relation_type),
        graph.vertices(), +[](ScoreState& state, typename Graph::VertexTopologyID vertex) {
          VertexData node = state.graph.getData(vertex);
          if (node.type != state.entity_type) {
            return;
          }
          pando::Vector<double> relationship_features =
              state.relationFeatures.getRelationFeature(state.relation_type);
          pando::Vector<float> scores_arr;
          PANDO_CHECK(scores_arr.initialize(relationship_features.size()));
          computeScore(scores_arr, state.head.features, relationship_features, node.features);
          PANDO_CHECK(
              state.scores.pushBack({computeL1Norm(scores_arr), state.graph.getTokenID(vertex)}));
          scores_arr.deinitialize();
        });

    galois::doAll(
        scores, +[](pando::GlobalRef<pando::Vector<ResultStruct>> localScores) {
          pando::Vector<ResultStruct> local = localScores;
          galois::merge_sort<ResultStruct>(local, [](ResultStruct a, ResultStruct b) {
            return a.first < b.first;
          });
          localScores = local;
        });

    pando::Vector<VertexTokenID> scores_final;
    int scores_final_size = 50;
    uint64_t numScores = scores.sizeAll();
    if (numScores < scores_final_size) {
      scores_final_size = numScores;
    }

    pando::Vector<uint64_t> offsets;
    PANDO_CHECK(offsets.initialize(scores.size()));

    // TODO(Patrick) Aggregate by host first, then reduce to top k
    PANDO_CHECK(scores_final.initialize(scores_final_size));
    for (int i = 0; i < scores_final_size; i++) {
      uint64_t thread = 0;
      uint64_t maxThread = thread;
      pando::Vector<ResultStruct> currVec = *scores.get(thread);
      galois::Pair<float, std::uint64_t> max = currVec[offsets[thread]];
      for (; thread < scores.size(); thread++) {
        currVec = *scores.get(thread);
        uint64_t offset = offsets[thread];
        if (offset >= currVec.size()) {
          continue;
        }
        galois::Pair<float, std::uint64_t> currResult = currVec[offset];
        if (currResult.second > max.second) {
          max = currResult;
          maxThread = thread;
        }
      }
      scores_final[i] = max.second;
      offsets[maxThread]++;
    }
    scores.deinitialize();
    offsets.deinitialize();
    return scores_final;
  }

  // TODO(Patrick) consider parallelizing this
  pando::Vector<VertexTokenID> computeVertexScores(Graph graph,
                                                   wf1::RelationFeatures relationFeatures,
                                                   pando::Vector<VertexTokenID>&& vertices,
                                                   std::uint64_t relation_type,
                                                   VertexTokenID head_id) {
    pando::Vector<ResultStruct> results;
    PANDO_CHECK(results.initialize(0));
    VertexData head = graph.getData(graph.getTopologyID(head_id));
    pando::Vector<double> relationship_features =
        relationFeatures.getRelationFeature(relation_type);
    pando::Vector<float> scores_arr;
    PANDO_CHECK(scores_arr.initialize(relationship_features.size()));
    for (VertexTokenID vertex : vertices) {
      VertexData vertexData = graph.getData(graph.getTopologyID(vertex));
      computeScore(scores_arr, head.features, relationship_features, vertexData.features);
      PANDO_CHECK(results.pushBack({computeL1Norm(scores_arr), vertex}));
    }
    scores_arr.deinitialize();

    galois::merge_sort<ResultStruct>(results, [](ResultStruct a, ResultStruct b) {
      return a.first < b.first;
    });
    for (uint64_t i = 0; i < vertices.size(); i++) {
      ResultStruct curr = results[i];
      vertices[i] = curr.second;
    }
    results.deinitialize();
    return vertices;
  }
};

} // namespace mhr_ref

#endif // PANDO_WF1_GALOIS_MHR_REF_HPP_
