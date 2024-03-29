// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#pragma once
#include <cstdint>

/**
 * base class for a task
 */
class task {
public:
  /**
   * default constructor empty
   */
  task() {}
  /**
   * default destructor empty; subclass can override
   */
  virtual ~task() {}
  /**
   * execute the task
   *
   * subclass must override
   */
  virtual void execute() = 0;
};

/**
 * task that wraps a function object
 */
template <typename F>
class task_base : public task {
public:
  /**
   * constructor
   */
  task_base(F f) : f_(f) {}
  /**
   * destructor
   */
  virtual ~task_base() {}
  /**
   * execute the task
   *
   * calls F's () operator
   */
  virtual void execute() {
    f_();
  }

private:
  F f_;
};

template <typename F>
static inline task* newTask(F f) {
  return new task_base<F>(f);
}

/**
 * execute this task on a specific core
 */
void execute_on(uint32_t pxn, uint32_t pod, uint32_t core, task* t);
