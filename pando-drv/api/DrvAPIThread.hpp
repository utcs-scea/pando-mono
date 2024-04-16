// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef DRV_API_THREAD_H
#define DRV_API_THREAD_H
#include <DrvAPIThreadState.hpp>
#include <DrvAPIMain.hpp>
#include <DrvAPISysConfig.hpp>
#include <DrvAPISystem.hpp>
#include <boost/coroutine2/all.hpp>
#include <memory>
namespace DrvAPI
{

enum phase_t {
    PHASE_INIT,
    PHASE_EXEC,
    PHASE_OTHER
};

class DrvAPIThread
{
public:
  using coro_t = boost::coroutines2::coroutine<void>;

  /**
   * @brief Construct a new DrvAPIThread object
   */
  DrvAPIThread();

  /**
   * @brief Destroy the DrvAPIThread object
   */
   ~DrvAPIThread(){}

  /**
   * @brief Start the thread
   */
  void start();

  /**
   * @brief Yield back to the main context
   *
   * @param state
   */
  void yield(DrvAPIThreadState *state);

  /**
   * @brief Yield back to the main context
   */
  void yield();

  /**
   * @brief Resume the thread context
   */
  void resume();

  /**
   * @brief Set the state object
   *
   * @param state
   */
  void setState(const std::shared_ptr<DrvAPIThreadState> &state) { state_ = state; }

  /**
   * @brief Get the state object
   *
   * @return std::shared_ptr<DrvAPIThreadState>
   */
  std::shared_ptr<DrvAPIThreadState> &getState() { return state_; }

  /**
   * @brief Set the phase
   *
   * @param phase
   */
  void setPhase(phase_t phase) { phase_ = phase;}

  /**
   * @brief Get the phase
   *
   * @return phase_t
   */
  phase_t getPhase() { return phase_; }

  /**
   * @brief Set the main function
   *
   * @param main
   */
  void setMain(drv_api_main_t main) { main_ = main; }

  /**
   * @brief Set argc & argv
   */
  void setArgs(int argc, char **argv) { argc_ = argc; argv_ = argv; }

  /**
   * @brief get the thread id
   */
  int id() const { return id_; } //!< Get the thread id

  /**
   * @brief get the thread id
   */
  int threadId() const { return id_; } //!< Get the thread id

  /**
   * @brief get the core id
   */
  int coreId() const { return core_id_; } //!< Get the core id

  /**
   * @brief number of threads on this core
   */
  int coreThreads() const { return core_threads_; } //!< Get the number of threads on this core

  /**
   * @brief set the number of threads on this core
   */
  void setCoreThreads(int threads) { core_threads_ = threads; } //!< Set the number of threads on this core

  /**
   * @brief set the core id
   */
  void setCoreId(int core_id) { core_id_ = core_id; } //!< Set the core id

  /**
   * @brief set the thread id
   */
  void setId(int id) { id_ = id; } //!< Set the thread id

  /**
   * @brief get the pod id w.r.t pxn
   */
  int podId() const { return pod_id_; } //!< Get the pod id w.r.t pxn

  /**
   * @brief set the pod id w.r.t pxn
   */
  void setPodId(int pod_id) { pod_id_ = pod_id; } //!< Set the pod id w.r.t pxn

  /**
   * @brief get the pxn id
   */
  int pxnId() const { return pxn_id_; } //!< Get the pxn id

  /**
   * @brief set the pxn id
   */
  void setPxnId(int pxn_id) { pxn_id_ = pxn_id; } //!< Set the pxn id

  /**
   * @brief using l1sp for stack
   */
  bool stackInL1SP() const {
      return stack_in_modeled_memory_;
  }

  /**
   * @brief using l1sp for stack
   */
  void setStackInL1SP(bool stack_in_l1sp) {
      stack_in_modeled_memory_ = stack_in_l1sp;
  }

  /**
   * @brief Get the system object
   *
   * @return std::shared_ptr<DrvAPISystem>
   */
  std::shared_ptr<DrvAPISystem> getSystem() const {
      return system_;
  }

  /**
   * @brief Set the system object
   *
   * @param sys
   */
  void setSystem(const std::shared_ptr<DrvAPISystem> &sys) {
      system_ = sys;
  }

  /**
   * @brief Set the execution tag
   *
   * @param tag
   * @return the old tag
   */
  int setTag(int tag);

  /**
   * @brief Get the execution tag
   *
   * @return the tag
   */
  int getTag() const;

  /**
   * @brief Convert a DrvAPIAddress to a native pointer
   */
  void addressToNative(DrvAPIAddress address, void **native, std::size_t *size);

  /**
   * @brief Convert a DrvAPIAddress to a native pointer
   */
  void nativeToAddress(void *native, DrvAPIAddress *address, std::size_t *size);

  /**
   * @brief Get the current active thread
   *
   * @return DrvAPIThread*
   */
  static DrvAPIThread* current() { return g_current_thread; } //!< Get the current active thread

  thread_local static DrvAPIThread *g_current_thread; //!< The current active thread

private:
  std::shared_ptr<DrvAPISystem> system_ = nullptr; //!< System object
  std::unique_ptr<coro_t::pull_type> thread_context_; //!< Thread context, coroutine that can be resumed
  coro_t::push_type *main_context_; //!< Main context, can be yielded back to
  std::shared_ptr<DrvAPIThreadState> state_; //!< Thread state
  phase_t phase_; //!< Phase
  drv_api_main_t main_; //!< Main function
  int argc_;
  char **argv_;
  int id_; //!< Thread id
  int core_id_; //!< Core id
  int core_threads_; //!< Number of threads on this core
  int pod_id_; //!< Pod id in PXN
  int pxn_id_; //!< Pxn id
  int tag_ = 0; //!< Execution tag
  bool stack_in_modeled_memory_ = false; //!< Stack is in modeled memory
};

/**
 * @brief Thread tag guard - restores old tag when out of scope
 */
class DrvAPITagGuard {
public:
    DrvAPITagGuard(int tag) {
        old_tag_ = DrvAPIThread::current()->setTag(tag);
    }
    DrvAPITagGuard(const DrvAPITagGuard &) = delete;
    DrvAPITagGuard &operator=(const DrvAPITagGuard &) = delete;
    DrvAPITagGuard(DrvAPITagGuard &&) = delete;
    DrvAPITagGuard &operator=(DrvAPITagGuard &&) = delete;

    ~DrvAPITagGuard() {
        DrvAPIThread::current()->setTag(old_tag_);
    }
    int old_tag_;
};

}

/**
 * @brief Get the current active thread
 *
 * @return DrvAPIThread*
 */
extern "C" DrvAPI::DrvAPIThread *DrvAPIGetCurrentContext();

/**
 * type definition for DrvAPIGetCurrentContext
 */
typedef DrvAPI::DrvAPIThread (*drv_api_get_thread_context_t)();

/**
 * @brief Set the current active thread
 *
 * @param thread
 */
extern "C" void DrvAPISetCurrentContext(DrvAPI::DrvAPIThread *thread);

/**
 * type definition for DrvAPISetCurrentContext
 */
typedef void (*drv_api_set_thread_context_t)(DrvAPI::DrvAPIThread *thread);
#endif
