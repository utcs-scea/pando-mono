// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF1_MHR_HPP_
#define PANDO_WF1_MHR_HPP_

#include <string>
#include <utility>

#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/containers/per_thread.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/sorts/merge_sort.hpp>
#include <pando-lib-galois/utility/pair.hpp>
#include <pando-rt/containers/vector.hpp>

void debugPrint(const std::string& msg, const std::string& file, int line,
                const std::string& func) {
  std::cout << "Debug: [" << file << ":" << line << " (" << func << ")] " << msg << std::endl;
}

#define DEBUG_PRINT(msg) debugPrint(msg, __FILE__, __LINE__, __func__)

namespace mhr {

template <typename Graph>
class MHR {
  using VertexTopologyID = std::uint64_t;
  using VertexTokenID = std::uint64_t;
  using ResultStruct = galois::Pair<float, pando::Vector<std::uint64_t>>;
  using VertexData = typename Graph::VertexData;

  static float computeScore(pando::Vector<std::uint64_t>& path, Graph graph) {
    float score = 0;
    for (auto id : path) {
      VertexData v = graph.getData(graph.getTopologyID(id));
      for (auto val : v.features) {
        if (val > 0)
          score += val;
        else
          score += -val;
      }
    }
    return score;
  }

public:
  pando::Vector<ResultStruct> GreedyReasoning(VertexTokenID S, VertexTopologyID T, Graph graph,
                                              uint64_t Lmax, uint64_t topK,
                                              uint64_t internal_topK) {
    VertexTokenID start_id = S;
    VertexTopologyID end_id = T;

    galois::PerThreadVector<ResultStruct> results;
    galois::PerThreadVector<pando::Vector<std::uint64_t>> new_paths;
    galois::PerThreadVector<pando::Vector<std::uint64_t>> old_paths;
    galois::PerThreadVector<ResultStruct> scores;

    PANDO_CHECK(results.initialize());
    PANDO_CHECK(new_paths.initialize());
    PANDO_CHECK(old_paths.initialize());
    PANDO_CHECK(scores.initialize());

    pando::Vector<std::uint64_t> vec1;
    PANDO_CHECK(vec1.initialize(0));
    PANDO_CHECK(vec1.pushBack(start_id));
    PANDO_CHECK(fmap(old_paths[0], pushBack, vec1));

    struct State {
      State() = default;
      State(galois::WaitGroup::HandleType f,
            galois::PerThreadVector<pando::Vector<std::uint64_t>> new_paths_, int L_max_,
            VertexTopologyID end_id_, Graph graph_, galois::PerThreadVector<ResultStruct> results_,
            galois::PerThreadVector<ResultStruct> scores_, uint64_t internal_topK_)
          : first(f),
            new_paths(new_paths_),
            L_max(L_max_),
            end_id(end_id_),
            graph(graph_),
            results(results_),
            scores(scores_),
            internal_topK(internal_topK_) {}

      galois::WaitGroup::HandleType first;
      galois::PerThreadVector<pando::Vector<std::uint64_t>> new_paths;
      int L_max;
      VertexTopologyID end_id;
      Graph graph;
      galois::PerThreadVector<ResultStruct> results;
      galois::PerThreadVector<ResultStruct> scores;
      uint64_t internal_topK;
    };

    while (old_paths.sizeAll() > 0) {
      scores.clear();
      galois::WaitGroup wg;
      State state(wg.getHandle(), new_paths, Lmax, end_id, graph, results, scores, internal_topK);
      galois::doAll(
          wg.getHandle(), state, old_paths,
          +[](State& state, pando::Vector<pando::Vector<std::uint64_t>> vec) {
            galois::doAll(
                state.first, state, vec,
                +[](State& state, pando::Vector<std::uint64_t> vec) {
                  if (vec.size() == state.L_max) {
                    return;
                  }
                  std::uint64_t last = vec[vec.size() - 1];
                  pando::Vector<ResultStruct> local_scores;
                  PANDO_CHECK(local_scores.initialize(0));
                  for (auto edgeid : state.graph.edges(state.graph.getTopologyID(last))) {
                    auto next = state.graph.getTokenID(state.graph.getEdgeDst(edgeid));
                    for (auto x : vec) {
                      if (x == next)
                        continue;
                    }
                    pando::Vector<std::uint64_t> cand_path;
                    PANDO_CHECK(cand_path.initialize(vec.size() + 1));
                    for (std::uint64_t i = 0; i < vec.size(); i++) {
                      cand_path[i] = vec[i];
                    }
                    cand_path[cand_path.size() - 1] = next;
                    float score = computeScore(cand_path, state.graph);
                    ResultStruct tmp{score, cand_path};
                    if (next == state.end_id) {
                      PANDO_CHECK(state.results.pushBack(tmp));
                    } else {
                      PANDO_CHECK(local_scores.pushBack(tmp));
                    }
                  }
                  galois::merge_sort<ResultStruct>(local_scores,
                                                   [](ResultStruct a, ResultStruct b) {
                                                     return a.first > b.first;
                                                   });
                  int local_scores_size = state.internal_topK;
                  if (local_scores.size() < state.internal_topK) {
                    local_scores_size = local_scores.size();
                  }
                  pando::Vector<ResultStruct> local_scores_final;
                  PANDO_CHECK(local_scores_final.initialize(local_scores_size));
                  for (uint64_t i = 0; i < local_scores_size; i++) {
                    local_scores_final[i] = local_scores[i];
                  }
                  for (ResultStruct score : local_scores_final) {
                    PANDO_CHECK(state.scores.pushBack(score));
                  }
                  local_scores.deinitialize();
                  local_scores_final.deinitialize();
                },
                +[](State state, pando::Vector<std::uint64_t> vec) {
                  std::uint64_t last = vec[vec.size() - 1];
                  return state.graph.getLocalityVertex(state.graph.getTopologyID(last));
                });
          });

      PANDO_CHECK(wg.wait());
      new_paths = state.new_paths;

      galois::DistArray<ResultStruct> scores_array;
      pando::Vector<ResultStruct> scores_vector;
      PANDO_CHECK(state.scores.assign(scores_array));
      PANDO_CHECK(scores_vector.initialize(scores_array.size()));
      for (uint64_t i = 0; i < scores_array.size(); i++) {
        scores_vector[i] = scores_array[i];
      }

      std::swap(new_paths, old_paths);
      new_paths.clear();
    }

    galois::DistArray<ResultStruct> results_array;
    PANDO_CHECK(results.assign(results_array));

    pando::Vector<ResultStruct> results_vector;
    PANDO_CHECK(results_vector.initialize(results_array.size()));

    for (uint64_t i = 0; i < results_array.size(); i++) {
      results_vector[i] = results_array[i];
    }
    galois::merge_sort<ResultStruct>(results_vector, [](ResultStruct a, ResultStruct b) {
      return a.first > b.first;
    });

    int vector_size = topK;
    if (results_vector.size() < topK) {
      vector_size = results_vector.size();
    }

    pando::Vector<ResultStruct> results_vector_final;
    PANDO_CHECK(results_vector_final.initialize(vector_size));

    for (std::uint64_t i = 0; i < vector_size; i++) {
      results_vector_final[i] = results_vector[i];
    }

    return results_vector_final;
  }
};
} // namespace mhr
#endif // PANDO_WF1_MHR_HPP_
