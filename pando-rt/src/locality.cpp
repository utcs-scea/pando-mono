// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "pando-rt/locality.hpp"

#if defined(PANDO_RT_USE_BACKEND_PREP)
#include "prep/cores.hpp"
#include "prep/hart_context_fwd.hpp"
#include "prep/nodes.hpp"
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
#include "drvx/drvx.hpp"
#endif // PANDO_RT_USE_BACKEND_PREP

namespace pando {

NodeIndex getCurrentNode() noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)

  return Nodes::getCurrentNode();

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  return Drvx::getCurrentNode();

#else

#error "Not implemented"

#endif // PANDO_RT_USE_BACKEND_PREP
}

NodeIndex getNodeDims() noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)

  return Nodes::getNodeDims();

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  return Drvx::getNodeDims();

#else

#error "Not implemented"

#endif // PANDO_RT_USE_BACKEND_PREP
}

PodIndex getCurrentPod() noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)

  return Cores::getCurrentPod();

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  return Drvx::getCurrentPod();

#else

#error "Not implemented"

#endif // PANDO_RT_USE_BACKEND_PREP
}

PodIndex getPodDims() noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)

  return Cores::getPodDims();

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  return Drvx::getPodDims();

#else

#error "Not implemented"

#endif // PANDO_RT_USE_BACKEND_PREP
}

CoreIndex getCurrentCore() noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)

  return Cores::getCurrentCore();

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  return Drvx::getCurrentCore();

#else

#error "Not implemented"

#endif // PANDO_RT_USE_BACKEND_PREP
}

CoreIndex getCoreDims() noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)

  auto coreDims = Cores::getCoreDims();

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  auto coreDims = Drvx::getCoreDims();

#else

#error "Not implemented"

#endif // PANDO_RT_USE_BACKEND_PREP

  // we use only one core / pod for scheduling, but the dimensions are 2D
  // reserve a column of cores for scheduling
  coreDims.x -= 1;
  return coreDims;
}

Place getCurrentPlace() noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)

  // PREP locality information requires expensive TLS checks
  Place place;
  place.node = Nodes::getCurrentNode();
  std::tie(place.pod, place.core) = Cores::getCurrentPodAndCore();
  return place;

#else

  return Place{getCurrentNode(), getCurrentPod(), getCurrentCore()};

#endif // PANDO_RT_USE_BACKEND_PREP
}

Place getPlaceDims() noexcept {
  return Place{getNodeDims(), getPodDims(), getCoreDims()};
}

ThreadIndex getCurrentThread() noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)

  return Cores::getCurrentThread();

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  return Drvx::getCurrentThread();

#else

#error "Not implemented"

#endif // PANDO_RT_USE_BACKEND_PREP
}

ThreadIndex getThreadDims() noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)

  return Cores::getThreadDims();

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  return Drvx::getThreadDims();

#else

#error "Not implemented"

#endif // PANDO_RT_USE_BACKEND_PREP
}

bool isOnCP() noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP)

  return hartContextGet() == nullptr;

#elif defined(PANDO_RT_USE_BACKEND_DRVX)

  return Drvx::isOnCP();

#else

#error "Not implemented"

#endif // PANDO_RT_USE_BACKEND_PREP
}

} // namespace pando
