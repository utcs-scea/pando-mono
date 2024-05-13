// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_UTILITY_LOCALITY_HPP_
#define PANDO_LIB_GALOIS_UTILITY_LOCALITY_HPP_

#include <pando-rt/export.h>

#include <cstdint>

#include <pando-lib-galois/utility/tuple.hpp>
#include <pando-rt/pando-rt.hpp>

namespace galois {
[[nodiscard]] inline std::uint64_t getThreadsPerCore() noexcept {
  std::uint64_t threads = static_cast<std::uint64_t>(pando::getThreadDims().id);
  return threads;
}

[[nodiscard]] inline std::uint64_t getThreadsPerPod() noexcept {
  const auto place = pando::getPlaceDims();
  const std::uint64_t cores =
      static_cast<std::uint64_t>(place.core.x) * static_cast<std::uint64_t>(place.core.y);
  return cores * getThreadsPerCore();
}

[[nodiscard]] inline std::uint64_t getThreadsPerHost() noexcept {
  const auto place = pando::getPlaceDims();
  const std::uint64_t pods =
      static_cast<std::uint64_t>(place.pod.x) * static_cast<std::uint64_t>(place.pod.y);
  return pods * getThreadsPerPod();
}

[[nodiscard]] inline std::uint64_t getNumThreads() noexcept {
  const auto place = pando::getPlaceDims();
  std::uint64_t nodes = static_cast<std::uint64_t>(place.node.id);
  return nodes * getThreadsPerHost();
}

[[nodiscard]] inline std::uint64_t getThreadIdxFromPlace(pando::Place place,
                                                         pando::ThreadIndex thread) noexcept {
  const auto placeDims = pando::getPlaceDims();
  const auto threadDims = pando::getThreadDims();
  const std::uint64_t threadsPerCore = static_cast<std::uint64_t>(threadDims.id);
  const std::uint64_t threadsPerPod = threadsPerCore *
                                      static_cast<std::uint64_t>(placeDims.core.x) *
                                      static_cast<std::uint64_t>(placeDims.core.y);
  const std::uint64_t threadsPerHost = threadsPerPod * static_cast<std::uint64_t>(placeDims.pod.x) *
                                       static_cast<std::uint64_t>(placeDims.pod.y);
  const std::uint64_t hostIdx = place.node.id;
  const std::uint64_t podIdx = place.pod.x * placeDims.pod.y + place.pod.y;
  const std::uint64_t coreIdx = place.core.x * placeDims.core.y + place.core.y;
  const std::uint64_t threadIdx = thread.id;
  return hostIdx * threadsPerHost + podIdx * threadsPerPod + coreIdx * threadsPerCore + threadIdx;
}

[[nodiscard]] inline galois::Tuple2<pando::Place, pando::ThreadIndex> getPlaceFromThreadIdx(
    std::uint64_t idx) noexcept {
  const auto placeDims = pando::getPlaceDims();
  const auto threadDims = pando::getThreadDims();
  const std::uint64_t threadsPerCore = static_cast<std::uint64_t>(threadDims.id);
  const std::uint64_t threadsPerPod = threadsPerCore *
                                      static_cast<std::uint64_t>(placeDims.core.x) *
                                      static_cast<std::uint64_t>(placeDims.core.y);
  const std::uint64_t threadsPerHost = threadsPerPod * static_cast<std::uint64_t>(placeDims.pod.x) *
                                       static_cast<std::uint64_t>(placeDims.pod.y);
  const pando::NodeIndex node(static_cast<int64_t>(idx / threadsPerHost));
  const std::uint64_t threadPerHostIdx = idx % threadsPerHost;
  const std::uint64_t podPerHostIdx = threadPerHostIdx / threadsPerPod;
  const pando::PodIndex pod(podPerHostIdx / placeDims.pod.y, podPerHostIdx % placeDims.pod.y);
  const std::uint64_t threadPerPodIdx = threadPerHostIdx % threadsPerPod;
  const std::uint64_t corePerPodIdx = threadPerPodIdx / threadsPerCore;
  const pando::CoreIndex core(corePerPodIdx / placeDims.core.y, corePerPodIdx % placeDims.core.y);
  const std::uint64_t threadPerCoreIdx = threadPerPodIdx % threadsPerCore;
  const pando::ThreadIndex thread(threadPerCoreIdx);
  return galois::make_tpl(pando::Place{node, pod, core}, thread);
}

[[nodiscard]] inline std::uint64_t getCurrentThreadIdx() noexcept {
  return getThreadIdxFromPlace(pando::getCurrentPlace(), pando::getCurrentThread());
}
} // namespace galois
#endif // PANDO_LIB_GALOIS_UTILITY_LOCALITY_HPP_
