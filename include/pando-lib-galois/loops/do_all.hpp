/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_LIB_GALOIS_LOOPS_DO_ALL_HPP_
#define PANDO_LIB_GALOIS_LOOPS_DO_ALL_HPP_
#include <pando-rt/export.h>

#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/global_ptr.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/notification.hpp>

namespace galois {

/**
 * @brief This function is used to interpose a barrier on do_all functors.
 *
 * @tparam F  the function type
 * @tparam State The type of the state to enable more state to be communicated
 * @tparam It the iterator pointing to the current object
 * @param[in] func the function be to run on the iterator
 * @param[in] s The state to pass into the function
 * @param[in] curr the iterator pointing to the current object
 * @param[in] done a pointer to a boolean to flag once done executing the function
 *
 * @TODO(AdityaAtulTewari) replace this with atomics.
 */
template <typename F, typename State, typename It>
void notifyFunc(const F& func, State s, It curr, pando::GlobalPtr<bool> done) {
  func(s, *curr);
  std::atomic_thread_fence(std::memory_order_release);
  *done = true;
}

template <typename F, typename It>
void notifyFunc(const F& func, It curr, pando::GlobalPtr<bool> done) {
  func(*curr);
  std::atomic_thread_fence(std::memory_order_release);
  *done = true;
}

/**
 * @brief This is the do_all loop from galois which takes a rang and lifts a function to it, and
 * adds a barrier afterwards.
 *
 * @tparam State the type of the state to enable closure variables to be communicated
 * @tparam R the range type, need to include begin, end, size
 * @tparam F the type of the functor.
 * @param[in] s the state to pass closure variables
 * @param[in] range a group of values to have func run on them.
 * @param[in] func the functor to lift
 */
template <typename State, typename R, typename F>
pando::Status doAll(State s, R& range, const F& func) {
  pando::Status err;
  pando::Array<bool> notifies;
  err = notifies.initialize(range.size());
  if (err != pando::Status::Success) {
    return err;
  }

  const auto end = range.end();

  err = pando::Status::Success;
  auto i = 0;
  for (auto curr = range.begin(); curr != range.end(); curr++) {
    err = pando::executeOn(pando::localityOf(curr), &notifyFunc<F, State, typename R::iterator>,
                           func, s, curr, &notifies[i]);
    if (err != pando::Status::Success) {
      notifies.deinitialize();
      return err;
    }
    i++;
  }
  for (auto notifref : notifies) {
    pando::waitUntil([notifref] {
      bool notify = notifref;
      return notify;
    });
  }
  notifies.deinitialize();
  return err;
}

template <typename F, typename Arg>
void bind(const F& func, Arg a) {
  func(a);
}

template <typename R, typename F>
pando::Status doAll(R& range, const F& func) {
  pando::Status err;
  pando::Array<bool> notifies;
  err = notifies.initialize(range.size());
  if (err != pando::Status::Success) {
    return err;
  }

  const auto end = range.end();

  err = pando::Status::Success;
  auto i = 0;
  for (auto curr = range.begin(); curr != range.end(); curr++) {
    err = pando::executeOn(pando::localityOf(curr), &notifyFunc<F, typename R::iterator>, func,
                           curr, &notifies[i]);
    if (err != pando::Status::Success) {
      notifies.deinitialize();
      return err;
    }
    i++;
  }
  for (auto notifref : notifies) {
    pando::waitUntil([notifref] {
      bool notify = notifref;
      return notify;
    });
  }
  notifies.deinitialize();
  return err;
}
} // namespace galois
#endif // PANDO_LIB_GALOIS_LOOPS_DO_ALL_HPP_
