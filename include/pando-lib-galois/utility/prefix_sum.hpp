// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_UTILITY_PREFIX_SUM_HPP_
#define PANDO_LIB_GALOIS_UTILITY_PREFIX_SUM_HPP_

#include <cstdint>
#include <variant>

#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/sync/waterfall_lock.hpp>
#include <pando-lib-galois/utility/counted_iterator.hpp>

namespace galois {

template <typename T>
struct PrefixState {
  PrefixState() = default;
  PrefixState(T prefixSum_, uint64_t numObjects_, uint64_t workers_)
      : prefixSum(prefixSum_), numObjects(numObjects_), workers(workers_) {}

  T prefixSum;
  uint64_t numObjects;
  uint64_t workers;
};

/** This function computes the indices for the different phases and forwards
 * them.
 * @param ns the number of items to sum
 * @param wf_id this corresponds to the thread id
 */
template <typename T>
void parallel_pfxsum_op(PrefixState<T>& state, uint64_t wf_id) {
  uint64_t ns = state.numObjects;
  uint64_t nt = state.prefixSum.numThreads;

  uint64_t div_sz = ns / (nt + 1);
  uint64_t bigs = ns % (nt + 1);
  uint64_t mid = nt >> 1;
  bool is_mid = mid == wf_id;
  // Concentrate the big in the middle thread
  uint64_t phase0_sz = is_mid ? div_sz + bigs : div_sz;
  uint64_t phase0_ind;
  if (wf_id <= mid)
    phase0_ind = div_sz * wf_id;
  else
    phase0_ind = bigs + (div_sz * wf_id);

  uint64_t phase2_sz = phase0_sz;
  uint64_t phase2_ind = wf_id ? phase0_ind : ns - div_sz;
  state.prefixSum.parallel_pfxsum_work(phase0_ind, phase0_sz, phase2_ind, phase2_sz, wf_id, nt);
}

inline void empty(std::monostate& a, uint64_t i) {
  (void)a;
  (void)i;
}

template <typename T>
inline T equalizer(T t) {
  return t;
}

template <typename T>
inline void before(T l, uint64_t tid) {
  l.template wait<1>(tid);
}
template <typename T>
inline void after(T l, uint64_t tid) {
  l.template done<2>(tid);
}

/** This is a struct used for repeated PrefixSums
 * It works using a 2 level algorithm
 * @param A The type of the source array
 * @param B The type of the dst array
 * @param transmute is function A -> B
 * @param scan_op is a function A x B -> B
 * @param combiner is a function B x B -> B
 * @param Conduit is the type used inside the WaterFallLock as well as the Paste
 * array (used for measurement)
 * @param src the source array user is required to ensure the size is correct
 * @param dst the destination array user is required to ensure the size is
 * correct
 * @param lock a reference to a WaterFallLock, which should have length of the
 * number of threads
 * @param paste a conduit assigned per thread in order to ensure cache_line
 * padding for speed
 */
template <typename A, typename B, typename A_Val, typename B_Val, B_Val (*transmute)(A_Val),
          B_Val (*scan_op)(A_Val x, B_Val y), B_Val (*combiner)(B_Val x, B_Val y),
          template <typename C> typename Conduit>
class PrefixSum {
public:
  /**
   * These are exposed in order to be changed between subsequent calls in the
   * case of dynamic structures
   */
  A src;
  B dst;

private:
  using PArr = Conduit<B_Val>;
  Conduit<B_Val> paste;
  using WFLType = galois::WaterFallLock<Conduit<unsigned>>;
  WFLType lock;

  /** The templates are used to make this function usable in many different
   * places Enables before and after to take in some context that can be
   * triggered after an object is in the paste array
   * @param Holder is used to specify any object holding A1 (A) or A2 (B)
   */
  template <typename A1, typename A2, typename A1_Val, typename A2_Val, A2_Val (*trans)(A1_Val),
            A2_Val (*scan)(A1_Val x, A2_Val y), typename CTX, void (*before)(CTX&, uint64_t),
            void (*after)(CTX&, uint64_t), bool combine = false>
  inline void serial_pfxsum(A1 src, A2 dst, uint64_t src_offset, uint64_t dst_offset, uint64_t ns,
                            CTX ctx) {
    if (!combine) {
      A1_Val src_val = src[src_offset + 0];
      dst[dst_offset + 0] = trans(src_val);
    }
    for (uint64_t i = 1; i < ns; i++) {
      before(ctx, i);
      A1_Val src_val = src[src_offset + i];
      A2_Val dst_val = dst[dst_offset + i - 1];
      dst[dst_offset + i] = scan(src_val, dst_val);
      after(ctx, i);
    }
  }

  /** Does the serial pfxsum and puts the final value in the paste_loc for
   * future processing
   *
   */
  inline void parallel_pfxsum_phase_0(uint64_t src_offset, uint64_t dst_offset, uint64_t ns,
                                      pando::GlobalRef<B_Val> paste_loc, uint64_t wfl_id) {
    serial_pfxsum<A, B, A_Val, B_Val, transmute, scan_op, std::monostate, empty, empty>(
        src, dst, src_offset, dst_offset, ns, std::monostate());
    paste_loc = dst[dst_offset + ns - 1];
    lock.template done<1>(wfl_id);
  }

  /** Sums up the paste locations in a single thread to prepare the finished
   * product
   *
   */
  inline void parallel_pfxsum_phase_1(uint64_t ns, uint64_t wfl_id) {
    (void)ns;
    if (!wfl_id) {
      lock.template done<2>(wfl_id);
      serial_pfxsum<PArr, PArr, B_Val, B_Val, equalizer, combiner, WFLType&, before<WFLType&>,
                    after<WFLType&>>(paste, paste, 0, 0, ns, lock);
    } else {
      lock.template wait<2>(wfl_id - 1);
    }
  }

  /** Does the final prefix sums with the last part of the array being handled
   *  by tid = 0
   */
  inline void parallel_pfxsum_phase_2(uint64_t src_offset, uint64_t dst_offset, uint64_t ns,
                                      B_Val phase1_val, bool pfxsum) {
    if (pfxsum) {
      A_Val src_val = src[src_offset + 0];
      dst[dst_offset + 0] = scan_op(src_val, phase1_val);
      serial_pfxsum<A, B, A_Val, B_Val, transmute, scan_op, std::monostate, empty, empty, true>(
          src, dst, src_offset, dst_offset, ns, std::monostate());
    } else {
      for (uint64_t i = 0; i < ns; i++) {
        B_Val dst_val = dst[dst_offset + i];
        dst[dst_offset + i] = combiner(phase1_val, dst_val);
      }
    }
  }

  inline void parallel_pfxsum_work(uint64_t phase0_ind, uint64_t phase0_sz, uint64_t phase2_ind,
                                   uint64_t phase2_sz, uint64_t wfl_id, uint64_t nt) {
    parallel_pfxsum_phase_0(phase0_ind, phase0_ind, phase0_sz, paste[wfl_id], wfl_id);

    parallel_pfxsum_phase_1(nt, wfl_id);

    B_Val paste_val = paste[wfl_id ? wfl_id - 1 : nt - 1];
    parallel_pfxsum_phase_2(phase2_ind, phase2_ind, phase2_sz, paste_val, !wfl_id);
  }

public:
  PrefixSum() = default;
  PrefixSum(A src_, B dst_) : src(src_), dst(dst_), paste(), lock() {}

  [[nodiscard]] pando::Status initialize(std::uint64_t numWorkers) {
    pando::Status err = lock.initialize(numWorkers);
    if (err != pando::Status::Success) {
      return err;
    }
    err = paste.initialize(numWorkers);
    if (err != pando::Status::Success) {
      return err;
    }
    return pando::Status::Success;
  }

  void deinitialize() {
    paste.deinitialize();
    lock.deinitialize();
  }

  /** computePrefixSum is the interface exposed to actually do a prefixSum
   * @param ns the number of objects in src to sum
   * @warning we expect ns to be less than equal to the length of source and destination
   */
  void computePrefixSum(uint64_t ns) {
    std::uint64_t workers = paste.size();
    uint64_t workPerThread = ns / (workers + 1);
    if (workPerThread <= 10) {
      workers /= pando::getThreadDims().id;
    }
    galois::doAllEvenlyPartition(
        PrefixState<PrefixSum<A, B, A_Val, B_Val, transmute, scan_op, combiner, Conduit>>(*this, ns,
                                                                                          workers),
        workers,
        +[](PrefixState<PrefixSum<A, B, A_Val, B_Val, transmute, scan_op, combiner, Conduit>>&
                state,
            uint64_t wf_id, uint64_t workers) {
          uint64_t ns = state.numObjects;
          uint64_t nt = workers;

          // optimize for smaller structures near the size of num_threads
          uint64_t div_sz = ns / (nt + 1);
          uint64_t bigs = ns % (nt + 1);
          uint64_t mid = nt >> 1;
          bool is_mid = mid == wf_id;
          // Concentrate the big in the middle thread
          uint64_t phase0_sz = is_mid ? div_sz + bigs : div_sz;
          uint64_t phase0_ind;
          if (wf_id <= mid)
            phase0_ind = div_sz * wf_id;
          else
            phase0_ind = bigs + (div_sz * wf_id);

          uint64_t phase2_sz = phase0_sz;
          uint64_t phase2_ind = wf_id ? phase0_ind : ns - div_sz;
          state.prefixSum.parallel_pfxsum_work(phase0_ind, phase0_sz, phase2_ind, phase2_sz, wf_id,
                                               nt);
        });
    this->lock.reset();
  }

  const char* name() {
    return typeid(PrefixSum<A, B, A_Val, B_Val, transmute, scan_op, combiner, Conduit>).name();
  }
};

} // namespace galois

#endif // PANDO_LIB_GALOIS_UTILITY_PREFIX_SUM_HPP_
