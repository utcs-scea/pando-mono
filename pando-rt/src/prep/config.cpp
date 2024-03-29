// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "config.hpp"

#include <cstdlib>
#include <limits>

#include "pando-rt/memory/address_map.hpp"

#include "log.hpp"
#include "status.hpp"

namespace pando {

namespace {

Config::Instance currentConfig;

} // namespace

Status Config::initialize() {
  // cores per pod

  if (auto cores = std::getenv("PANDO_PREP_NUM_CORES"); cores != nullptr) {
    currentConfig.compute.coreCount = std::atoll(cores);
  }
  const std::int64_t maxCoresPerPod =
      (0x1LU << addressMap.L1SP.coreX.width()) * (0x1LU << addressMap.L1SP.coreY.width());
  if ((currentConfig.compute.coreCount < 1) || (currentConfig.compute.coreCount > maxCoresPerPod)) {
    SPDLOG_ERROR("Cores/pod should be in the range [1, {}]. Provided value: {}", maxCoresPerPod,
                 currentConfig.compute.coreCount);
    return Status::OutOfBounds;
  }

  // harts per core

  if (auto harts = std::getenv("PANDO_PREP_NUM_HARTS"); harts != nullptr) {
    currentConfig.compute.hartCount = std::atoll(harts);
  }
  const std::int64_t maxHartsPerCore = std::numeric_limits<decltype(ThreadIndex::id)>::max();
  if ((currentConfig.compute.hartCount < 1) ||
      (currentConfig.compute.hartCount > maxHartsPerCore)) {
    SPDLOG_ERROR("Harts/core should be in the range [1, {}]. Provided value: {}", maxHartsPerCore,
                 currentConfig.compute.hartCount);
    return Status::OutOfBounds;
  }

  // L1SP per hart

  if (auto stack = std::getenv("PANDO_PREP_L1SP_HART"); stack != nullptr) {
    currentConfig.memory.l1SPHart = std::atoll(stack);
  }
  const std::int64_t maxL1SPPerHart = (0x1LU << addressMap.L1SP.offset.width());
  if (currentConfig.memory.l1SPHart > maxL1SPPerHart) {
    SPDLOG_ERROR("L1SP/hart should be in the range [0, {}]. Provided value: {}", maxL1SPPerHart,
                 currentConfig.memory.l1SPHart);
    return Status::OutOfBounds;
  }

  // L2SP per pod

  if (auto l2SP = std::getenv("PANDO_PREP_L2SP_POD"); l2SP != nullptr) {
    currentConfig.memory.l2SPPod = std::atoll(l2SP);
  }
  const std::int64_t maxL2SPPerPod = (0x1LU << addressMap.L2SP.offset.width());
  if (currentConfig.memory.l2SPPod > maxL2SPPerPod) {
    SPDLOG_ERROR("L2SP/pod should be in the range [0, {}]. Provided value: {}", maxL2SPPerPod,
                 currentConfig.memory.l2SPPod);
    return Status::OutOfBounds;
  }

  // Main memory per node

  if (auto mainMemory = std::getenv("PANDO_PREP_MAIN_NODE"); mainMemory != nullptr) {
    currentConfig.memory.mainNode = std::atoll(mainMemory);
  }
  const std::int64_t maxMainPerNode = (0x1LU << addressMap.main.offset.width());
  if (currentConfig.memory.mainNode > maxMainPerNode) {
    SPDLOG_ERROR("Main/node should be in the range [0, {}]. Provided value: {}", maxMainPerNode,
                 currentConfig.memory.mainNode);
    return Status::OutOfBounds;
  }

  SPDLOG_INFO(
      "PXN configuration: cores/pod={}, harts/core={}, L1SP/hart (thread stack)={}, L2SP/pod={}, "
      "Main Memory/node={}",
      currentConfig.compute.coreCount, currentConfig.compute.hartCount,
      currentConfig.memory.l1SPHart, currentConfig.memory.l2SPPod, currentConfig.memory.mainNode);

  return Status::Success;
}

const Config::Instance& Config::getCurrentConfig() noexcept {
  return currentConfig;
}

} // namespace pando
