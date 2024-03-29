// SPDX-License-Identifier: MIT
/* Copyright (c) 2023-2024 Advanced Micro Devices, Inc. All rights reserved. */

#include "pando-rt/execution/execute_on_impl.hpp"

#include "pando-rt/stdlib.hpp"

#if defined(PANDO_RT_USE_BACKEND_PREP)
#include "prep/cores.hpp"
#include "prep/hart_context_fwd.hpp"
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
#include "drvx/cores.hpp"
#include "drvx/drvx.hpp"
#endif // PANDO_RT_USE_BACKEND_PREP

namespace pando {

Status detail::executeOn(Place place, Task task) {
  // check if node index is within bounds
  const auto& nodeDims = getNodeDims();
  if ((place.node < NodeIndex{0}) || (place.node >= nodeDims)) {
    PANDO_ABORT("Invalid node index");
  }

  // check if pod index is within bounds
  const auto& podDims = getPodDims();
  if ((place.pod < PodIndex{0, 0}) || (place.pod.x >= podDims.x || place.pod.y >= podDims.y)) {
    PANDO_ABORT("Invalid pod index");
  }

  const auto& coreDims = getCoreDims();
  if (place.core == anyCore) {
    // anyCore: get scheduler core queue
    const CoreIndex schedulerCoreIndex(coreDims.x, 0);
    place.core = schedulerCoreIndex;
  } else {
    if ((place.core < CoreIndex{0, 0}) ||
        (place.core.x >= coreDims.x || place.core.y >= coreDims.y)) {
      PANDO_ABORT("Invalid core index");
    }
  }

#if defined(PANDO_RT_USE_BACKEND_PREP) || defined(PANDO_RT_USE_BACKEND_DRVX)

  auto* queue = Cores::getTaskQueue(place);
  const auto result = queue->enqueue(std::move(task));
  hartYield();
  return result;

#else

#error "Not implemented"

#endif // PANDO_RT_USE_BACKEND_PREP || PANDO_RT_USE_BACKEND_DRVX
}

} // namespace pando
