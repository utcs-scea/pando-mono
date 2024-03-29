// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <cstdio>
#include <pando-rt/pando-rt.hpp>

// Prints information (macro to avoid extra stack)
#define PRINT_INFORMATION(message, place)                                                          \
  do {                                                                                             \
    const auto stackUsage = pando::getThreadStackSize() - pando::getThreadAvailableStack();        \
    std::printf(                                                                                   \
        "%s: Node %i, pod x=%i,y=%i, core x=%i,y=%i, total stack(bytes): %lu, used stack(bytes): " \
        "%lu, available stack(bytes): %lu\n",                                                      \
        (message), (place).node.id, (place).pod.x, (place).pod.y, (place).core.x, (place).core.y,  \
        pando::getThreadStackSize(), stackUsage, pando::getThreadAvailableStack());                \
  } while (false)

// Prints the stack size for a task created via executeOn
void printStackSize() {
  const auto& thisPlace = pando::getCurrentPlace();
  PRINT_INFORMATION("executeOn", thisPlace);
}

// Prints the stack size for a task created via executeOn that will create another task
void printNestedStackSize() {
  const auto& thisPlace = pando::getCurrentPlace();
  PRINT_INFORMATION("executeOn w/ nested call", thisPlace);
  PANDO_CHECK(pando::executeOn(thisPlace, &printStackSize));
}

// Prints the stack size for a task created via executeOnWait
void printStackSizeBlocking() {
  const auto& thisPlace = pando::getCurrentPlace();
  PRINT_INFORMATION("executeOnWait", thisPlace);
}

// Prints the stack size for a task created via executeOnWait that will create another task
void printNestedStackSizeBlocking() {
  const auto& thisPlace = pando::getCurrentPlace();
  PRINT_INFORMATION("executeOnWait w/ nested call", thisPlace);
  auto result = pando::executeOnWait(thisPlace, &printStackSizeBlocking);
  pando::waitUntil([&] {
    return result.hasValue();
  });
}

int pandoMain(int, char**) {
  const auto& thisPlace = pando::getCurrentPlace();
  const auto& placeDims = pando::getPlaceDims();
  if (thisPlace.node == pando::NodeIndex(0)) {
    std::printf("Configuration (nodes, pods, cores): (%i), (%i,%i), (%i,%i)\n", placeDims.node.id,
                placeDims.pod.x, placeDims.pod.y, placeDims.core.x, placeDims.core.y);
  }

  const pando::Place place{thisPlace.node, pando::anyPod, pando::anyCore};

  // executeOn stack sizes
  {
    PANDO_CHECK(pando::executeOn(place, &printStackSize));
    pando::waitAll();
  }
  {
    PANDO_CHECK(pando::executeOn(place, &printNestedStackSize));
    pando::waitAll();
  }

  // executeOnWait stack sizes
  {
    auto result = pando::executeOnWait(place, &printStackSizeBlocking);
    pando::waitUntil([&] {
      return result.hasValue();
    });
    pando::waitAll();
  }

  {
    auto result = pando::executeOnWait(place, &printNestedStackSizeBlocking);
    pando::waitUntil([&] {
      return result.hasValue();
    });
    pando::waitAll();
  }

  return 0;
}
