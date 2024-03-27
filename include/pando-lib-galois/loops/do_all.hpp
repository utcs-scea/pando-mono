// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_LOOPS_DO_ALL_HPP_
#define PANDO_LIB_GALOIS_LOOPS_DO_ALL_HPP_

#include <pando-rt/export.h>

#include <pando-lib-galois/sync/wait_group.hpp>
#include <pando-lib-galois/utility/counted_iterator.hpp>
#include <pando-rt/containers/vector.hpp>
#include <pando-rt/memory/global_ptr.hpp>
#include <pando-rt/pando-rt.hpp>

namespace galois {

/**
 * @brief the generic localityOf function for pointers for DoAll
 */
template <typename T>
pando::Place localityOf(pando::GlobalPtr<T> ptr) {
  return (ptr == nullptr) ? pando::getCurrentPlace() : pando::localityOf(ptr);
}

template <typename T>
pando::Place localityOf(pando::Array<T> ptr) {
  return pando::localityOf(ptr.data());
}

static inline uint64_t getTotalThreads() {
  uint64_t coreY = pando::getPlaceDims().core.y;
  uint64_t cores = pando::getPlaceDims().core.x * coreY;
  uint64_t threads = pando::getThreadDims().id;
  uint64_t hosts = pando::getPlaceDims().node.id;
  return hosts * cores * threads;
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
  template <typename F, typename State>
  static void notifyFuncOnEach(const F& func, State s, galois::IotaRange::iterator curr,
                               uint64_t totalThreads, WaitGroup::HandleType wgh) {
    func(s, *curr, totalThreads);
    wgh.done();
  }

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
   * adds a barrier afterwards, also supports a custom locality function.
   * @warning May be deprecated after December
   *
   * @tparam State the type of the state to enable closure variables to be communicated
   * @tparam R the range type, need to include begin, end, size
   * @tparam F the type of the functor.
   * @tparam L the type of the locality functor.
   *
   * @param[in] wgh   A WaitGroup that is used to detect termination
   * @param[in] s     the state to pass closure variables
   * @param[in] range a group of values to have func run on them.
   * @param[in] func  the functor to lift
   */
  template <typename State, typename R, typename F, typename L>
  static pando::Status doAll(WaitGroup::HandleType wgh, State s, R& range, const F& func,
                             const L& localityFunc) {
    pando::Status err = pando::Status::Success;

    const auto end = range.end();

    std::uint64_t iter = 0;
    std::uint64_t size = range.size();
    wgh.add(size);

    for (auto curr = range.begin(); curr != end; curr++) {
      // Required hack without workstealing
      auto nodePlace = pando::Place{localityFunc(s, *curr).node, pando::anyPod, pando::anyCore};
      err = pando::executeOn(nodePlace, &notifyFunc<F, State, typename R::iterator>, func, s, curr,
                             wgh);
      if (err != pando::Status::Success) {
        for (std::uint64_t i = iter; i < size; i++) {
          wgh.done();
        }
        return err;
      }
      iter++;
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
  template <typename State, typename R, typename F>
  static pando::Status doAll(WaitGroup::HandleType wgh, State s, R& range, const F& func) {
    pando::Status err = pando::Status::Success;

    const auto end = range.end();

    std::uint64_t iter = 0;
    std::uint64_t size = range.size();
    wgh.add(size);

    for (auto curr = range.begin(); curr != end; curr++) {
      // Required hack without workstealing
      auto nodePlace = pando::Place{localityOf(curr).node, pando::anyPod, pando::anyCore};
      err = pando::executeOn(nodePlace, &notifyFunc<F, State, typename R::iterator>, func, s, curr,
                             wgh);
      if (err != pando::Status::Success) {
        for (std::uint64_t i = iter; i < size; i++) {
          wgh.done();
        }
        return err;
      }
      iter++;
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

    std::uint64_t iter = 0;
    const std::uint64_t size = range.size();
    wgh.add(size);

    for (auto curr = range.begin(); curr != end; curr++) {
      // Required hack without workstealing
      auto nodePlace = pando::Place{localityOf(curr).node, pando::anyPod, pando::anyCore};
      err = pando::executeOn(nodePlace, &notifyFunc<F, typename R::iterator>, func, curr, wgh);
      if (err != pando::Status::Success) {
        for (std::uint64_t i = iter; i < size; i++) {
          wgh.done();
        }
        return err;
      }
      iter++;
    }
    return err;
  }

  /**
   * @brief This is the do_all loop from galois which takes a range and lifts a function to it, and
   * adds a barrier afterwards.
   * @warning May be deprecated after December
   *
   * @tparam State the type of the state to enable closure variables to be communicated
   * @tparam R the range type, need to include begin, end, size
   * @tparam F the type of the functor.
   * @param[in] s the state to pass closure variables
   * @param[in] range a group of values to have func run on them.
   * @param[in] func the functor to lift
   */
  template <typename State, typename R, typename F, typename L>
  static pando::Status doAll(State s, R& range, const F& func, const L& localityFunc) {
    pando::Status err;
    WaitGroup wg;
    err = wg.initialize(0);
    if (err != pando::Status::Success) {
      return err;
    }
    doAll<State, R, F>(wg.getHandle(), s, range, func, localityFunc);
    err = wg.wait();
    wg.deinitialize();
    return err;
  }

  /**
   * @brief This is the do_all loop from galois which takes a range and lifts a function to it, and
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
    wg.deinitialize();
    return err;
  }

  /**
   * @brief This is the do_all loop from galois which takes an integer number of work items and
   * lifts a function to it, and adds a barrier afterwards.  Work is spread evenly across all pxns
   *
   * @tparam State the type of the state to enable closure variables to be communicated
   * @tparam F the type of the functor.
   *
   * @param[in] wgh   A WaitGroup that is used to detect termination
   * @param[in] s     the state to pass closure variables
   * @param[in] workItems the amount of work that needs to be spawned
   * @param[in] func  the functor to lift
   */
  template <typename State, typename F>
  static pando::Status doAllEvenlyPartition(WaitGroup::HandleType wgh, State s, uint64_t workItems,
                                            const F& func) {
    pando::Status err = pando::Status::Success;
    if (workItems == 0) {
      return err;
    }

    uint64_t hosts = pando::getPlaceDims().node.id;
    uint64_t workPerHost = workItems / hosts;
    galois::IotaRange range(0, workItems);
    const auto end = range.end();

    for (auto curr = range.begin(); curr != end; curr++) {
      int16_t assignedHost;
      if (workPerHost > 0) {
        assignedHost = (int16_t)(*curr / workPerHost);
      } else if (hosts % workItems == 0) {
        // Given that we have 8 pxns and we schedule 4 workItems, this will place the workers on the
        // even numbered pxns
        assignedHost = (int16_t)(*curr * (hosts / workItems));
      } else {
        // Given that workItems < hosts and workItems does not divide hosts, assign the work
        // sequentially
        assignedHost = (int16_t)(*curr);
      }
      if (assignedHost >= pando::getPlaceDims().node.id) {
        assignedHost = pando::getPlaceDims().node.id - 1;
      }
      wgh.addOne();
      // Required hack without workstealing
      auto nodePlace = pando::Place{pando::NodeIndex(assignedHost), pando::anyPod, pando::anyCore};
      err = pando::executeOn(nodePlace, &notifyFuncOnEach<F, State>, func, s, curr, workItems, wgh);
      if (err != pando::Status::Success) {
        wgh.done();
        return err;
      }
    }
    return err;
  }

  /**
   * @brief This is the do_all loop from galois which takes an integer number of work items and
   * lifts a function to it, and adds a barrier afterwards.  Work is spread evenly across all pxns
   *
   * @tparam State the type of the state to enable closure variables to be communicated
   * @tparam F the type of the functor.
   * @param[in] s the state to pass closure variables
   * @param[in] workItems the amount of work that needs to be spawned
   * @param[in] func the functor to lift
   */
  template <typename State, typename F>
  static pando::Status doAllEvenlyPartition(State s, uint64_t workItems, const F& func) {
    pando::Status err;
    WaitGroup wg;
    err = wg.initialize(0);
    if (err != pando::Status::Success) {
      return err;
    }
    doAllEvenlyPartition<State, F>(wg.getHandle(), s, workItems, func);
    err = wg.wait();
    wg.deinitialize();
    return err;
  }

  /**
   * @brief This is the on_each loop from Galois it lifts a function and runs it on all threads in
   * the cluster, and adds a barrier afterwards.
   *
   * @tparam State the type of the state to enable closure variables to be communicated
   * @tparam F the type of the functor.
   *
   * @param[in] wgh   A WaitGroup that is used to detect termination
   * @param[in] s     the state to pass closure variables
   * @param[in] func  the functor to lift
   */
  template <typename State, typename F>
  static pando::Status onEach(WaitGroup::HandleType wgh, State s, const F& func) {
    return doAllEvenlyPartition<State, F>(wgh, s, getTotalThreads(), func);
  }

  /**
   * @brief This is the on_each loop from Galois it lifts a function and runs it on all threads in
   * the cluster, and adds a barrier afterwards.
   *
   * @tparam State the type of the state to enable closure variables to be communicated
   * @tparam F the type of the functor.
   * @param[in] s the state to pass closure variables
   * @param[in] func the functor to lift
   */
  template <typename State, typename F>
  static pando::Status onEach(State s, const F& func) {
    pando::Status err;
    WaitGroup wg;
    err = wg.initialize(0);
    if (err != pando::Status::Success) {
      return err;
    }
    onEach<State, F>(wg.getHandle(), s, func);
    err = wg.wait();
    wg.deinitialize();
    return err;
  }
};

template <typename State, typename R, typename F, typename L>
pando::Status doAll(WaitGroup::HandleType wgh, State s, R range, const F& func,
                    const L& localityFunc) {
  return DoAll::doAll<State, R, F, L>(wgh, s, range, func, localityFunc);
}
template <typename State, typename R, typename F>
pando::Status doAll(WaitGroup::HandleType wgh, State s, R range, const F& func) {
  return DoAll::doAll<State, R, F>(wgh, s, range, func);
}
template <typename R, typename F>
pando::Status doAll(WaitGroup::HandleType wgh, R range, const F& func) {
  return DoAll::doAll<R, F>(wgh, range, func);
}
template <typename State, typename R, typename F, typename L>
pando::Status doAll(State s, R range, const F& func, const L& localityFunc) {
  return DoAll::doAll<State, R, F, L>(s, range, func, localityFunc);
}
template <typename State, typename R, typename F>
pando::Status doAll(State s, R range, const F& func) {
  return DoAll::doAll<State, R, F>(s, range, func);
}
template <typename R, typename F>
pando::Status doAll(R range, const F& func) {
  return DoAll::doAll<R, F>(range, func);
}

template <typename State, typename F>
pando::Status doAllEvenlyPartition(WaitGroup::HandleType wgh, State s, uint64_t workItems,
                                   const F& func) {
  return DoAll::doAllEvenlyPartition<State, F>(wgh, s, workItems, func);
}
template <typename State, typename F>
pando::Status doAllEvenlyPartition(State s, uint64_t workItems, const F& func) {
  return DoAll::doAllEvenlyPartition<State, F>(s, workItems, func);
}

template <typename State, typename F>
pando::Status onEach(WaitGroup::HandleType wgh, State s, const F& func) {
  return DoAll::onEach<State, F>(wgh, s, func);
}
template <typename State, typename F>
pando::Status onEach(State s, const F& func) {
  return DoAll::onEach<State, F>(s, func);
}

} // namespace galois
#endif // PANDO_LIB_GALOIS_LOOPS_DO_ALL_HPP_
