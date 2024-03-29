// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#pragma once
#include "DrvAPIThread.hpp"
#include <memory>
namespace SST {
namespace Drv {

// forward declarations
class DrvCore;

/**
 * Forward thread
 */
class DrvThread {
public:
  /**
   * Constructor
   */
  DrvThread();

  // Default destructor
  ~DrvThread() = default;

  // No copy
  DrvThread(const DrvThread&) = delete;
  DrvThread& operator=(const DrvThread&) = delete;

  // Move only
  DrvThread(DrvThread&&) = default;
  DrvThread& operator=(DrvThread&&) = default;

  /**
   * Execute the thread
   */
  void execute(DrvCore *core);

  /**
   * Get the api thread
   */
  DrvAPI::DrvAPIThread &getAPIThread() { return *thread_; }

  /**
   * Get the api thread
   */
  const DrvAPI::DrvAPIThread &getAPIThread() const { return *thread_; }

private:
  std::unique_ptr<DrvAPI::DrvAPIThread> thread_;
};

}
}
