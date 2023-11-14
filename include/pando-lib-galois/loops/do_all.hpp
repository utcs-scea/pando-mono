// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_LOOPS_DO_ALL_HPP_
#define PANDO_LIB_GALOIS_LOOPS_DO_ALL_HPP_

#include <pando-rt/export.h>

#include <pando-lib-galois/sync/wait_group.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/global_ptr.hpp>
#include <pando-rt/pando-rt.hpp>

namespace galois {

/**
 * @brief the generic localityOf function for pointers for DoAll
 */
template <typename T>
pando::Place localityOf(pando::GlobalPtr<T> ptr) {
  return pando::localityOf(ptr);
}

class DoAll {
private:
  /**
   * @brief This function is used to interpose a barrier on do_all functors.
   *
   * @tparam F  the function type
   * @tparam State The type of the state to enable more state to be communicated
   * @tparam It the iterator pointing to the current object
   * @param[in] func the function be to run on the iterator
   * @param[in] s The state to pass into the function
   * @param[in] curr the iterator pointing to the current object
   * @param[in] wgh A WaitGroup handle to notify the task of completion
   *
   */
  template <typename F, typename State, typename It>
  static void notifyFunc(const F& func, State s, It curr, WaitGroup::HandleType wgh) {
    func(s, *curr);
    wgh.done();
  }

  /**
   * @brief This function is used to interpose a barrier on do_all functors.
   *
   * @tparam F  the function type
   * @tparam It the iterator pointing to the current object
   * @param[in] func the function be to run on the iterator
   * @param[in] curr the iterator pointing to the current object
   * @param[in] wgh A WaitGroup handle to notify the task of completion
   *
   */
  template <typename F, typename It>
  static void notifyFunc(const F& func, It curr, WaitGroup::HandleType wgh) {
    func(*curr);
    wgh.done();
  }

public:
  /**
   * @brief This is the do_all loop from galois which takes a rang and lifts a function to it, and
   * adds a barrier afterwards.
   *
   * @tparam State the type of the state to enable closure variables to be communicated
   * @tparam R the range type, need to include begin, end, size
   * @tparam F the type of the functor.
   *
   * @param[in] wgh   A WaitGroup that is used to detect termination
   * @param[in] s     the state to pass closure variables
   * @param[in] range a group of values to have func run on them.
   * @param[in] func  the functor to lift
   */
  template <typename State, typename R, typename F>
  static pando::Status doAll(WaitGroup::HandleType wgh, State s, R& range, const F& func) {
    pando::Status err = pando::Status::Success;

    const auto end = range.end();

    for (auto curr = range.begin(); curr != end; curr++) {
      wgh.addOne();
      err = pando::executeOn(localityOf(curr), &notifyFunc<F, State, typename R::iterator>, func, s,
                             curr, wgh);
      if (err != pando::Status::Success) {
        wgh.done();
        return err;
      }
    }
    return err;
  }

  /**
   * @brief This is the do_all loop from galois which takes a rang and lifts a function to it, and
   * adds a barrier afterwards.
   *
   * @tparam State the type of the state to enable closure variables to be communicated
   * @tparam R the range type, need to include begin, end, size
   * @tparam F the type of the functor.
   *
   * @param[in] wgh   A WaitGroup that is used to detect termination
   * @param[in] s     the state to pass closure variables
   * @param[in] range a group of values to have func run on them.
   * @param[in] func  the functor to lift
   */
  template <typename R, typename F>
  static pando::Status doAll(WaitGroup::HandleType wgh, R& range, const F& func) {
    pando::Status err = pando::Status::Success;

    const auto end = range.end();

    for (auto curr = range.begin(); curr != end; curr++) {
      wgh.addOne();
      err =
          pando::executeOn(localityOf(curr), &notifyFunc<F, typename R::iterator>, func, curr, wgh);
      if (err != pando::Status::Success) {
        wgh.done();
        return err;
      }
    }
    return err;
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
  static pando::Status doAll(State s, R& range, const F& func) {
    pando::Status err;
    WaitGroup wg;
    err = wg.initialize(0);
    if (err != pando::Status::Success) {
      return err;
    }
    doAll<State, R, F>(wg.getHandle(), s, range, func);
    err = wg.wait();
    if (err != pando::Status::Success) {
      return err;
    }
    wg.deinitialize();
    return err;
  }

  /**
   * @brief This is the do_all loop from galois which takes a range and lifts a function to it, and
   * adds a barrier afterwards.
   *
   * @tparam R the range type, need to include begin, end, size
   * @tparam F the type of the functor.
   * @param[in] range a group of values to have func run on them.
   * @param[in] func the functor to lift
   */
  template <typename R, typename F>
  static pando::Status doAll(R& range, const F& func) {
    pando::Status err;
    WaitGroup wg;
    err = wg.initialize(0);
    if (err != pando::Status::Success) {
      return err;
    }
    doAll<R, F>(wg.getHandle(), range, func);

    err = wg.wait();
    if (err != pando::Status::Success) {
      return err;
    }
    wg.deinitialize();
    return err;
  }
};

template <typename State, typename R, typename F>
pando::Status doAll(WaitGroup::HandleType wgh, State s, R range, const F& func) {
  return DoAll::doAll<State, R, F>(wgh, s, range, func);
}
template <typename R, typename F>
pando::Status doAll(WaitGroup::HandleType wgh, R range, const F& func) {
  return DoAll::doAll<R, F>(wgh, range, func);
}
template <typename State, typename R, typename F>
pando::Status doAll(State s, R range, const F& func) {
  return DoAll::doAll<State, R, F>(s, range, func);
}
template <typename R, typename F>
pando::Status doAll(R range, const F& func) {
  return DoAll::doAll<R, F>(range, func);
}

} // namespace galois
#endif // PANDO_LIB_GALOIS_LOOPS_DO_ALL_HPP_
