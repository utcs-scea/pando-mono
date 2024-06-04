// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_WF1_MINIBATCHER_HPP_
#define PANDO_WF1_MINIBATCHER_HPP_

#include <ctime>

#include <pando-lib-galois/containers/dist_array.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/utility/dist_accumulator.hpp>

#include <pando-wf1/gnntypes.hpp>

namespace gnn {

/**
 * @brief Minibatch generator.
 *
 * @details This generates minibatches. First, it copies target vertex IDs which
 * are used for training; these are sampled vertices in the current code.
 * Second, it sorts the IDs randomly. Third, this fetches and
 * returns minibatch from the sorted IDs of a given minibatch size. The third phase
 * is repeated until all vertex IDs are used for training.
 * When all the vertices are used for training, this is one epoch. At the next epoch,
 * it sorts the ID again, and fetches minibatches from the beginning.
 */
template <typename InnerGraph>
class MinibatchGenerator {
public:
  constexpr MinibatchGenerator() noexcept = default;
  ~MinibatchGenerator() noexcept = default;
  constexpr MinibatchGenerator(MinibatchGenerator<InnerGraph>&&) noexcept = default;
  constexpr MinibatchGenerator(const MinibatchGenerator<InnerGraph>&) noexcept = default;
  constexpr MinibatchGenerator<InnerGraph>& operator=(
      const MinibatchGenerator<InnerGraph>&) noexcept = default;
  constexpr MinibatchGenerator<InnerGraph>& operator=(MinibatchGenerator<InnerGraph>&&) noexcept =
      default;

  void initialize(const galois::HostLocalStorage<pando::Array<bool>>& targetMask,
                  galois::HostLocalStorage<pando::Array<bool>>& minibatchMask,
                  VertexDenseID minibatchSize, const InnerGraph& graph) {
    std::cout << "[Minibatcher] Starts minibatcher initialization\n" << std::flush;
    this->minibatchMask_ = minibatchMask;
    this->InitializeVertexIDArray(graph, targetMask);
    this->InitializePerHostMinibatchState(minibatchSize);
    std::cout << "[Minibatcher] Starts minibatcher initialization [DONE]\n" << std::flush;
  }

  /**
   * @brief Initialize per-host minibatch size.
   */
  void InitializePerHostMinibatchState(VertexDenseID minibatchSize) {
    struct Tpl {
      VertexDenseID minibatchSize;
      galois::HostLocalStorage<VertexDenseID> currentPoint;
    };

    PANDO_CHECK(this->minibatchSize_.initialize());
    PANDO_CHECK(this->currentPoint_.initialize());

    galois::doAll(
        Tpl{minibatchSize, this->currentPoint_}, this->minibatchSize_,
        +[](Tpl tpl, pando::GlobalRef<VertexDenseID> minibatchSize) {
          VertexDenseID totalMsz = tpl.minibatchSize;
          std::uint32_t host = pando::getCurrentPlace().node.id;
          std::uint32_t numHosts = pando::getPlaceDims().node.id;
          VertexDenseID msz = totalMsz / numHosts;
          msz += (totalMsz % numHosts > host) ? 1 : 0;
          minibatchSize = msz;
          *fmap(tpl.currentPoint, get, host) = 0;
        });
  }

  /**
   * @brief Aggregates sampled vertex IDs.
   *
   * @details Before this method call, each vertex had already been assigned one vertex type
   * between training, testing, and validation (on GNNGraph).
   * This method traverses a local graph and aggregates IDs of the vertices
   * of which types match to the current phase.
   */
  void InitializeVertexIDArray(const InnerGraph& graph,
                               const galois::HostLocalStorage<pando::Array<bool>>& targetMask) {
    PANDO_CHECK(this->vertexSet_.initialize());

    struct Tpl {
      InnerGraph g;
      galois::HostLocalStorage<pando::Array<bool>> targetMask;
    };

    galois::doAll(
        Tpl{graph, targetMask}, this->vertexSet_,
        +[](Tpl tpl, pando::GlobalRef<pando::Vector<VertexDenseID>> tv) {
          std::uint32_t host = pando::getCurrentPlace().node.id;
          InnerGraph g = tpl.g;
          pando::Array<bool> mask = *tpl.targetMask.get(host);
          PANDO_CHECK(fmap(tv, initialize, 0));
          for (VertexDenseID v = 0; v < fmap(g, localSize, host); ++v) {
            if (mask[v]) {
              PANDO_CHECK(fmap(tv, pushBack, v));
            }
          }
        });
  }

  /**
   * @brief Shuffle vertex IDs for next minibatches.
   *
   * @details The minibatcher class maintains a vertex ID vector containing sampled vertex IDs for
   * the current phase. This method shuffles the vector; Minibatcher advances a pointer to
   * the first index of the next minibatch, starting from index 0. This method randomizes
   * the minibatches and avoids local minima during training, as well as bias in testing. This
   * method also resets the per-host minibatch pointers to 0.
   */
  void ResetMinibatching() {
    galois::doAll(
        this->currentPoint_, this->vertexSet_,
        +[](galois::HostLocalStorage<VertexDenseID> cPtr,
            pando::GlobalRef<pando::Vector<VertexDenseID>> tvRef) {
          std::uint32_t host = pando::getCurrentPlace().node.id;

          // Reset the minibatch pointer to 0
          *fmap(cPtr, get, host) = 0;

          // Shuffle vertex IDs
          pando::Vector<VertexDenseID> tv = tvRef;
          for (VertexDenseID i = tv.size() - 1; i > 0; --i) {
            std::uint32_t seed = time(nullptr);
            std::mt19937 rndGen = std::mt19937(rand_r(&seed));
            std::uniform_int_distribution<VertexDenseID> distr(0, i);
            VertexDenseID j = distr(rndGen);
            VertexDenseID tmp = tv[i];
            tv[i] = tv[j];
            tv[j] = tmp;
          }
        });
    /*
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      pando::GlobalRef<pando::Vector<VertexDenseID>> tvRef = fmap(this->vertexSet_, get, host);
      pando::Vector<VertexDenseID> tv = tvRef;
      std::cout << fmap(this->currentPoint_, get, host) << "\n";
      for (VertexDenseID i = 0; i < tv.size(); ++i) {
        std::cout << "host:" << host << ", i:" << i << ", reset minib:" << tv[i] << "\n";
      }
    }
    */
  }

  /**
   * @brief Selects vertices for the next minibatch.
   *
   * @details A minibatch is generated by marking elements at indices corresponding to
   * minibatched vertex dense IDs (It is essential that this mask is shuffled in the preceding
   * phase). This method initializes the mask and the pointer, and sets elements at indices
   * corresponding to the next vertex IDs.
   */
  void GetNextMinibatch() {
    struct Tpl {
      galois::HostLocalStorage<pando::Vector<VertexDenseID>> tvs;
      galois::HostLocalStorage<VertexDenseID> points;
      galois::HostLocalStorage<VertexDenseID> mbSz;
    };
    galois::doAll(
        Tpl{this->vertexSet_, this->currentPoint_, this->minibatchSize_}, this->minibatchMask_,
        +[](Tpl tpl, pando::GlobalRef<pando::Array<bool>> maskRef) {
          // Reset a mask
          pando::Array<bool> mask = maskRef;
          galois::doAll(
              mask, +[](pando::GlobalRef<bool> v) {
                v = false;
              });

          std::uint32_t host = pando::getCurrentPlace().node.id;

          pando::GlobalRef<VertexDenseID> point = *fmap(tpl.points, get, host);
          pando::Vector<VertexDenseID> tv = *fmap(tpl.tvs, get, host);
          VertexDenseID mbSz = *fmap(tpl.mbSz, get, host);

          // Update a mask to the next batch
          std::uint64_t index{0};
          while (index < tv.size()) {
            mask[tv[VertexDenseID(point++)]] = true;
            ++index;
            // If a complete minibatch is created, stop it
            if (index == mbSz) {
              break;
            }
            // If a minibatch is not fully filled but there is no further vertex, stop it
            if (point == tv.size()) {
              break;
            }
          }
        });
#if 0
    for (std::uint32_t host = 0; host < static_cast<std::uint32_t>(pando::getPlaceDims().node.id);
         ++host) {
      pando::GlobalRef<pando::Array<bool>> maskRef = fmap(this->minibatchMask_, get, host);
      pando::GlobalRef<pando::Vector<VertexDenseID>> tv = fmap(this->vertexSet_, get, host);
      pando::Array<bool> mask = maskRef;
      std::cout << "Point:" << fmap(this->currentPoint_, get, host) << "\n";
      for (VertexDenseID i = 0; i < lift(mask, size); ++i) {
        std::cout << "host:" << host << ", i:" << i << ", vid:" << mask[i] << "\n";
      }
    }
#endif
  }

  /**
   * @brief Return true if all vertices chosen for the current phase (between training,
   * testing, and validation) have been minibatched. Otherwise return false.
   */
  bool NoMoreMinibatching() {
    bool noMoreWork{true};
    for (std::uint32_t host = 0; host < pando::getPlaceDims().node.id; ++host) {
      std::uint64_t currPoint = *fmap(this->currentPoint_, get, host);
      pando::Vector<VertexDenseID> tv = *fmap(this->vertexSet_, get, host);
      std::uint64_t localVertexNum = lift(tv, size);
      if (currPoint < localVertexNum) {
        noMoreWork = false;
        break;
      }
    }
    return noMoreWork;
  }

private:
  /// @brief Per-host minibatched vertex mask
  galois::HostLocalStorage<pando::Array<bool>> minibatchMask_;
  /// @brief Per-host minibatched vertices
  galois::HostLocalStorage<pando::Vector<VertexDenseID>> vertexSet_;
  /// @brief Per-host minibatch size
  galois::HostLocalStorage<VertexDenseID> minibatchSize_;
  /// @brief Per-host pointer to the first vertex of the next minibatch
  galois::HostLocalStorage<VertexDenseID> currentPoint_;
};

} // namespace gnn

#endif // PANDO_WF1_MINIBATCHER_HPP_
