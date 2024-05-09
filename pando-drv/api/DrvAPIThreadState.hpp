// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef DRV_API_THREAD_STATE_H
#define DRV_API_THREAD_STATE_H
#include <DrvAPIAddress.hpp>
#include <DrvAPIReadModifyWrite.hpp>
#include <DrvAPISystem.hpp>
#include <utility>
#include <stdlib.h>
namespace DrvAPI
{

enum stage_t {
    STAGE_INIT,
    STAGE_EXEC_COMP,
    STAGE_EXEC_COMM,
    STAGE_OTHER
};

/**
 * @brief The thread state
 *
 */
class DrvAPIThreadState
{
public:
  DrvAPIThreadState() {}
  virtual ~DrvAPIThreadState() {}
  virtual bool canResume() const { return true; }
};

/**
 * @brief The thread state for an idle thread
 *
 */
class DrvAPIThreadIdle : public DrvAPIThreadState
{
public:
  DrvAPIThreadIdle() {}
};

/**
 * @brief Terminated thread state
 */
class DrvAPITerminate : public DrvAPIThreadState
{
public:
  DrvAPITerminate() {}
  bool canResume() const override { return false; }
};

/**
 * @brief No-op thread state
 */
class DrvAPINop : public DrvAPIThreadState
{
public:
    DrvAPINop(int count) : can_resume_(false), count_(count) {}
    bool canResume() const override { return can_resume_; }
    int count() const { return count_; }
    void complete(){ can_resume_ = true; }
private:
    bool can_resume_;
    int  count_;
};

/**
 * @brief Set stage thread state
 */
class DrvAPISetStage : public DrvAPIThreadState
{
public:
    DrvAPISetStage(stage_t stage) : can_resume_(false), stage_(stage) {}
    bool canResume() const override { return can_resume_; }
    void complete(){ can_resume_ = true; }
    stage_t getStage() const { return stage_; }
private:
    bool can_resume_;
    stage_t  stage_;
};

/**
 * @brief Increment Phase thread state
 */
class DrvAPIIncrementPhase : public DrvAPIThreadState
{
public:
    DrvAPIIncrementPhase() : can_resume_(false) {}
    bool canResume() const override { return can_resume_; }
    void complete(){ can_resume_ = true; }
private:
    int can_resume_;
};

/**
 * @brief Base thread state for a memory operation
 *
 */
class DrvAPIMem : public DrvAPIThreadState
{
public:
  DrvAPIMem(DrvAPIAddress address);

  virtual bool canResume() const  { return can_resume_; }

  void complete() { can_resume_ = true; }
  DrvAPIAddress getAddress() const { return address_; }

protected:
  bool can_resume_;
  DrvAPIAddress address_;
};

/**
 * @brief Base thread state for a memory read
 *
 * @tparam T
 */
class DrvAPIMemRead : public DrvAPIMem
{
public:
  DrvAPIMemRead(DrvAPIAddress address)
    : DrvAPIMem(address){}


  virtual void getResult(void *p) = 0;
  virtual void setResult(void *p) = 0;
  virtual size_t getSize() const { return 0; }
};

/**
 * @brief Concrete thread state for a memory read
 *
 * @tparam T
 */
template <typename T>
class DrvAPIMemReadConcrete : public DrvAPIMemRead
{
public:
  DrvAPIMemReadConcrete(DrvAPIAddress address)
      : DrvAPIMemRead(address) {}

  virtual void getResult(void *p) override {
    *static_cast<T*>(p) = value_;
  }

  virtual void setResult(void *p) override {
    value_ = *static_cast<T*>(p);
  }

  virtual size_t getSize() const override {
    return sizeof(T);
  }

private:
  T value_;
};

/**
 * @brief Base thread state for a memory write
 *
 * @tparam T
 */
class DrvAPIMemWrite : public DrvAPIMem
{
public:
  DrvAPIMemWrite(DrvAPIAddress address)
    : DrvAPIMem(address) {}
  virtual void getPayload(void *p) = 0;
  virtual void setPayload(void *p)  = 0;
  virtual size_t getSize() const { return 0; }
};

/**
 * @brief Concrete thread state for a memory write
 *
 * @tparam T
 */
template <typename T>
class DrvAPIMemWriteConcrete : public DrvAPIMemWrite
{
public:
  DrvAPIMemWriteConcrete(DrvAPIAddress address, T value)
      : DrvAPIMemWrite(address), value_(value) {}
  virtual void getPayload(void *p) override {
    *static_cast<T*>(p) = value_;
  }

  virtual void setPayload(void *p) override {
    value_ = *static_cast<T*>(p);
  }

  virtual size_t getSize() const override {
    return sizeof(T);
  }

private:
  T value_;
};


/**
 * @brief Base thread state for an atomic read-modify-write
 */
class DrvAPIMemAtomic : public DrvAPIMem {
public:
  DrvAPIMemAtomic(DrvAPIAddress address)
    : DrvAPIMem(address) {}
  virtual void getPayload(void *p) = 0;
  virtual void setPayload(void *p)  = 0;
  virtual void getResult(void *p) = 0;
  virtual void setResult(void *p) = 0;
  /*
   * Extended payload for atomic operations with two operands
   */
  virtual void getPayloadExt(void *p) {}
  virtual void setPayloadExt(void *p) {}
  virtual bool hasExt() const { return false; }
  virtual void modify() = 0;
  virtual size_t getSize() const { return 0; }
  virtual DrvAPIMemAtomicType getOp() const = 0;
};

/**
 * @brief Concrete thread state for an atomic read-modify-write
 *
 * @tparam T The data type.
 * @tparam op The operation.
 */
template <typename T, DrvAPIMemAtomicType OP>
class DrvAPIMemAtomicConcrete : public DrvAPIMemAtomic {
public:
  DrvAPIMemAtomicConcrete(DrvAPIAddress address, T value)
    : DrvAPIMemAtomic(address), w_value_(value) {}
  void getPayload(void *p) override {
    *static_cast<T*>(p) = w_value_;
  }
  void setPayload(void *p) override {
    w_value_ = *static_cast<T*>(p);
  }
  void getResult(void *p) override {
    *static_cast<T*>(p) = r_value_;
  }
  void setResult(void *p) override {
    r_value_  = *static_cast<T*>(p);
  }
  void modify() override {
    std::tie(w_value_, r_value_) = atomic_modify<T>(w_value_, r_value_, OP);
  }
  size_t getSize() const override { return sizeof(T); }
  DrvAPIMemAtomicType getOp() const override { return OP; }
private:
  T r_value_;
  T w_value_;
};

/**
 * @brief Concrete thread state for an atomic read-modify-write
 *
 * @tparam T The data type.
 * @tparam op The operation.
 */
template <typename T, DrvAPIMemAtomicType OP>
class DrvAPIMemAtomicConcreteExt : public DrvAPIMemAtomic {
public:
  DrvAPIMemAtomicConcreteExt(DrvAPIAddress address, T value, T ext)
      : DrvAPIMemAtomic(address), w_value_(value), ext_value_(ext) {}

  void getPayload(void *p) override {
    *static_cast<T*>(p) = w_value_;
  }
  void setPayload(void *p) override {
    w_value_ = *static_cast<T*>(p);
  }
  void getResult(void *p) override {
    *static_cast<T*>(p) = r_value_;
  }
  void setResult(void *p) override {
    r_value_  = *static_cast<T*>(p);
  }
  void getPayloadExt(void *p) override {
    *static_cast<T*>(p) = ext_value_;
  }
  void setPayloadExt(void *p) override {
    ext_value_ = *static_cast<T*>(p);
  }
  bool hasExt() const override {
    return true;
  }
  void modify() override {
      std::tie(w_value_,r_value_) = atomic_modify<T>(w_value_, r_value_, ext_value_, OP);
  }
  size_t getSize() const override { return sizeof(T); }
  DrvAPIMemAtomicType getOp() const override { return OP; }
private:
  T r_value_;
  T w_value_;
  T ext_value_;
};


/**
 * @brief Request to the simulator to convert a DrvAPIAddress to a native pointer
 *
 * WARNING:
 * This state type will not work in multi-rank simulations.
 * This state type may not work depending on the memory model used.
 * This state type may not work depending on the memory controller used.
 *
 * Avoid using this function if possible. But if you need to use it, it's here.
 * But use it at your own risk, and don't expect it to work for all memory models
 * and simulation configurations.
 */
class DrvAPIToNativePointer : public DrvAPIMem
{
public:
    /**
     * @brief Construct a new DrvAPI To Native Pointer object
     */
    DrvAPIToNativePointer(DrvAPIAddress address)
        : DrvAPIMem(address)
        , native_pointer_(nullptr)
        , region_size_(0) {
    }

    /**
     * @brief Get the native pointer
     *
     * @return void*
     */
    void *getNativePointer() const {
        return native_pointer_;
    }

    /**
     * @brief Set the native pointer
     *
     * @param p
     */
    void setNativePointer(void *p) {
        native_pointer_ = p;
    }

    /**
     * @brief Get the size of the memory region
     *
     * @return std::size_t
     */
    std::size_t getRegionSize() const {
        return region_size_;
    }

    /**
     * @brief Set the size of the memory region
     *
     * @param size
     */
    void setRegionSize(std::size_t size) {
        region_size_ = size;
    }
private:
    void *native_pointer_;  //!< the native pointer
    std::size_t region_size_;      //!< the size of the memory region
};

}
#endif
