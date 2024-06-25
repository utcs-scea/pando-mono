// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <cstdio>
#include <pando-rt/pando-rt.hpp>

void greetings(int level) {
  auto thisPlace = pando::getCurrentPlace();
  auto placeDims = pando::getPlaceDims();

  if (level < 2) {
    pando::CoreIndex neighborCore{
        static_cast<std::int8_t>((thisPlace.core.x + 1) % placeDims.core.x), thisPlace.core.y};
    pando::Place otherPlace{thisPlace.node, thisPlace.pod, neighborCore};
    PANDO_CHECK(pando::executeOn(otherPlace, &greetings, level + 1));
  }

  std::printf("%s/%i: Hello from node %li, pod x=%i,y=%i, core x=%i,y=%i\n", "greetings", level,
              thisPlace.node.id, thisPlace.pod.x, thisPlace.pod.y, thisPlace.core.x,
              thisPlace.core.y);
}

void nodeGreetings(int level) {
  auto thisPlace = pando::getCurrentPlace();
  auto placeDims = pando::getPlaceDims();

  if (level < 2) {
    pando::NodeIndex rightNode{
        static_cast<std::int64_t>((thisPlace.node.id + 1) % placeDims.node.id)};
    PANDO_CHECK(
        pando::executeOn(pando::Place{rightNode, {}, pando::anyCore}, &nodeGreetings, level + 1));
  }
  std::printf("%s/%i: Hello from node %li, pod x=%i,y=%i, core x=%i,y=%i\n", "nodeGreetings", level,
              thisPlace.node.id, thisPlace.pod.x, thisPlace.pod.y, thisPlace.core.x,
              thisPlace.core.y);
}

int pandoMain(int, char**) {
  const auto placeDims = pando::getPlaceDims();
  std::printf("Configuration (nodes, pods, cores): (%li), (%i,%i), (%i,%i)\n", placeDims.node.id,
              placeDims.pod.x, placeDims.pod.y, placeDims.core.x, placeDims.core.y);

  const auto thisPlace = pando::getCurrentPlace();

  // execute on this node
  if (thisPlace.node.id == 0) {
    PANDO_CHECK(pando::executeOn(pando::Place{}, &greetings, 0));
  }

  pando::waitAll();

  // execute on right node
  if (thisPlace.node.id == 0) {
    pando::NodeIndex rightNode((thisPlace.node.id + 1) % pando::getPlaceDims().node.id);
    PANDO_CHECK(pando::executeOn(pando::Place{rightNode, {}, pando::anyCore}, &nodeGreetings, 0));
  }

  pando::endExecution();

  return 0;
}
