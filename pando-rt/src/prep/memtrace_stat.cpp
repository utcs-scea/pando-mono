// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include "memtrace_stat.hpp"

#include <cstdlib>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "log.hpp"
#include "pando-rt/locality.hpp"

namespace pando {

namespace {

std::uint64_t countInGranularity(std::uint64_t bytes) {
  return ((bytes + 15) / 16);
}

struct MemStat {
  std::uint64_t bytes{};
  std::uint64_t bytes_granularity{};
  std::uint64_t count_granularity{};
  std::uint64_t count_operations{};
};

std::ofstream stat_file;
std::mutex mutex;
std::unique_ptr<std::unordered_map<std::string, MemStat>[]> stats;
std::uint32_t phaseCount = 1;
NodeIndex numOfNodes;
bool isDirty = false;
} // namespace

[[nodiscard]] Status MemTraceStat::initialize(NodeIndex nodeIdx, NodeIndex nodeDims) {
  if (!isOnCP()) {
    SPDLOG_ERROR("MemTraceStat can only be initialized from the CP");
    return Status::Error;
  }

  numOfNodes = nodeDims;

  std::string prefix = "";

  if (auto filePrefixEnv = std::getenv("PANDO_TRACING_MEM_STAT_FILE_PREFIX");
      filePrefixEnv != nullptr) {
    prefix = std::string(filePrefixEnv) + "_";
  }

  std::string filePath = prefix + "pando_mem_stat_node_" + std::to_string(nodeIdx.id) + ".trace";

  stat_file.open(filePath, std::ios::out | std::ios::trunc);

  if (!stat_file.is_open()) {
    return Status::Error;
  }

  stats = std::make_unique<std::unordered_map<std::string, MemStat>[]>(nodeDims.id);

  stat_file << "Destination Node: " << nodeIdx.id << "\n\n";

  return Status::Success;
}

void MemTraceStat::add(std::string_view op, pando::NodeIndex other, std::size_t size) {
  if (other >= numOfNodes) {
    SPDLOG_ERROR("Node index out of bounds: {}", other.id);
    return;
  }

  std::lock_guard<std::mutex> lock(mutex);

  auto key = std::string(op);
  auto& htable = stats[other.id];
  auto it = htable.find(key);
  auto cGranularity = countInGranularity(size);
  if (it != htable.end()) {
    it->second.bytes += size;
    it->second.count_operations += 1;
    it->second.bytes_granularity += cGranularity * 16;
    it->second.count_granularity += cGranularity;
  } else {
    MemStat stat;
    stat.bytes = size;
    stat.count_operations = 1;
    stat.bytes_granularity = cGranularity * 16;
    stat.count_granularity = cGranularity;
    htable[key] = stat;
  }

  isDirty = true;
}

void MemTraceStat::startKernel(std::string_view kernelName) {
  // trying to start a new kernel section but there's data still
  // from the previous phase, waiting to be written, so flush them first
  if (isDirty) {
    writePhase();
  }

  std::lock_guard<std::mutex> lock(mutex);

  stat_file << "### Kernel: " << kernelName << " ###\n";
  phaseCount = 1;
}

void MemTraceStat::writePhase() {
  std::lock_guard<std::mutex> lock(mutex);

  if (!isDirty)
    return;

  stat_file << "Phase: " << phaseCount << "\n";

  for (int i = 0; i < numOfNodes.id; ++i) {
    stat_file << "Source Node: " << i << "\n";
    for (auto& pair : stats[i]) {
      if (pair.second.count_operations == 0) // do not log stats with zero values
        continue;

      // write the count stat to the file
      stat_file << pair.first << " (count): " << pair.second.count_operations << "\n";
      // write the count granularity stat to the file
      stat_file << pair.first << " (count - 16B granularity): " << pair.second.count_granularity
                << "\n";
      // write the bytes stat to the file
      stat_file << pair.first << " (bytes): " << pair.second.bytes << "\n";
      stat_file << pair.first << " (bytes - 16B granularity): " << pair.second.bytes_granularity
                << "\n";

      // reset the stat
      pair.second = MemStat();
    }
  }

  stat_file << "\n";

  stat_file.flush();

  phaseCount++;

  isDirty = false;
}

void MemTraceStat::finalize() {
  if (!isOnCP()) {
    SPDLOG_ERROR("MemTraceStat can only be finalized from the CP");
    return;
  }

  // write the final statistics
  writePhase();

  stat_file.close();
}
}; // namespace pando
