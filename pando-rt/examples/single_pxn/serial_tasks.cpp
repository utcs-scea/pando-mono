// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <cstdio>
#include <pando-rt/pando-rt.hpp>

/** @file serial_tasks.cpp
 * This is a simple serial task test.
 * Each core sequentially increments a single integer value.
 */

/**
 * @brief Increment an integer value by 1.
 */
void incrementValue(pando::GlobalPtr<std::int16_t> sharedValue,
                    pando::Notification::HandleType handle) {
  *sharedValue = *sharedValue + 1;
  handle.notify();
}

int pandoMain(int, char**) {
  const auto placeDims = pando::getPlaceDims();
  std::printf("Configuration (nodes, pods, cores): (%i), (%i,%i), (%i,%i)\n", placeDims.node.id,
              placeDims.pod.x, placeDims.pod.y, placeDims.core.x, placeDims.core.y);

  if (placeDims.core.x == 0 || placeDims.core.y == 0) {
    std::printf("# core should be > 0\n");
    pando::exit(EXIT_FAILURE);
  }

  if (placeDims.pod.x == 0 || placeDims.pod.y == 0) {
    std::printf("# pod should be > 0\n");
    pando::exit(EXIT_FAILURE);
  }

  const auto thisPlace = pando::getCurrentPlace();
  std::int16_t numCores = placeDims.core.x * placeDims.core.y;
  auto mmResource = pando::getDefaultMainMemoryResource();
  auto sharedValue =
      static_cast<pando::GlobalPtr<std::int16_t>>(mmResource->allocate(sizeof(std::int16_t)));
  *sharedValue = 0;
  for (std::int8_t ix = 0; ix < placeDims.core.x; ++ix) {
    for (std::int8_t iy = 0; iy < placeDims.core.y; ++iy) {
      pando::Notification notification;
      PANDO_CHECK(notification.init());
      pando::CoreIndex core{ix, iy};
      pando::Place corePlace{thisPlace.node, thisPlace.pod, core};
      PANDO_CHECK(
          pando::executeOn(corePlace, &incrementValue, sharedValue, notification.getHandle()));
      // Wait until the current iterating core is done.
      if (!notification.waitFor(std::chrono::seconds(10))) {
        // 124 == Timedout code
        std::printf("Waiting a core failed: TIMEOUT\n");
        pando::exit(124);
      }
    }
  }

  if (*sharedValue == numCores) {
    std::printf("Succeeded.\n");
  } else {
    std::printf("Failed.\n");
    pando::exit(EXIT_FAILURE);
  }

  mmResource->deallocate(sharedValue, sizeof(std::int16_t));

  return 0;
}
