// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_EXECUTION_TASK_HPP_
#define PANDO_RT_EXECUTION_TASK_HPP_

#include <algorithm>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

#include "../memory.hpp"
#include "../memory/global_ptr.hpp"
#include "../stddef.hpp"
#include "../sync/atomic.hpp"
#include "../execution/termination.hpp"

namespace pando {

namespace detail {

/**
 * @brief Function object wrapper for function pointers.
 *
 * @ingroup ROOT
 */
template <typename F>
struct FunctionWrapper {
  F funPtr;

  constexpr explicit FunctionWrapper(F funPtr) noexcept : funPtr(funPtr) {}

  template <typename... T>
  decltype(auto) operator()(T&&... t) {
    return (*funPtr)(std::forward<T>(t)...);
  }
};

/**
 * @brief Returns a type for storing object of type @p F.
 *
 * @ingroup ROOT
 */
template <typename F>
using FunctionStorage =
    std::conditional_t<std::is_function_v<std::remove_pointer_t<F>>, FunctionWrapper<F>, F>;

} // namespace detail

/**
 * @brief Function wrapper for asynchronous task invocation.
 *
 * Instances of this class can store, move, and invoke callable targets, i.e., functions, lambda
 * expressions, and function objects. A small object optimization mechanism makes sure that
 * allocations are avoided as much as possible.
 *
 * @note A @ref Task object can be invoked at most once as the stored arguments are moved into the
 *       target.
 *
 * @ingroup ROOT
 */
class Task {
  struct WithPostamble {};
  struct WithResultPtr {};

public:
  /// @brief Tag to indicate that the target will be followed by a postamble.
  static constexpr WithPostamble withPostamble{};

  /// @brief Tag to indicate that the target returns a value that will be assigned to a pointer.
  static constexpr WithResultPtr withResultPtr{};

private:
  static constexpr std::size_t SmallObjectStorageSize =
      std::max(alignof(std::max_align_t), 2 * sizeof(void*));

  // Vtable to access the type-erased target
  struct VTable {
    void (*dtor)(void*) noexcept; // NOLINT(readability/casting) unnamed arg
    void (*destructiveMove)(void*, void*);
    void (*invoke)(void*);
  };

  // Target with no return value
  // Callable and arguments are stored using inheritance to allow EBO
  template <typename F, typename... Args>
  struct Target : private detail::FunctionStorage<F>, private std::tuple<Args...> {
    using FunctionStorageType = detail::FunctionStorage<F>;
    using ArgsStorageType = std::tuple<Args...>;

    static void dtor(void* self) noexcept {
      static_cast<Target*>(self)->~Target();
    }

    static void destructiveMove(void* self, void* p) {
      new (p) Target(std::move(*static_cast<Target*>(self)));
      static_cast<Target*>(self)->~Target();
    }

    static void invoke(void* self) {
      static_cast<Target*>(self)->operator()();
    }

    static constexpr VTable vtable = {dtor, destructiveMove, invoke};

    template <typename TF, typename... TArgs,
              std::enable_if_t<!std::is_same_v<std::decay_t<TF>, Target>, int> = 0>
    constexpr explicit Target(TF&& f, TArgs&&... args)
        : FunctionStorageType(std::forward<TF>(f)), ArgsStorageType(std::forward<TArgs>(args)...) {}

    constexpr void operator()() {
      std::apply(static_cast<FunctionStorageType&>(*this),
                 std::move(static_cast<ArgsStorageType&>(*this)));
      if (!isOnCP()) {
        TerminationDetection::increaseTasksFinished(1);
      }
    }
  };

  // Target with postamble and no return value
  // PostAmble, callable and arguments are stored using inheritance to allow EBO
  template <typename Postamble, typename F, typename... Args>
  struct TargetWithPostamble : private detail::FunctionStorage<Postamble>,
                               private detail::FunctionStorage<F>,
                               private std::tuple<Args...> {
    using PostambleStorageType = detail::FunctionStorage<Postamble>;
    using FunctionStorageType = detail::FunctionStorage<F>;
    using ArgsStorageType = std::tuple<Args...>;

    static void dtor(void* self) noexcept {
      static_cast<TargetWithPostamble*>(self)->~TargetWithPostamble();
    }

    static void destructiveMove(void* self, void* p) {
      new (p) TargetWithPostamble(std::move(*static_cast<TargetWithPostamble*>(self)));
      static_cast<TargetWithPostamble*>(self)->~TargetWithPostamble();
    }

    static void invoke(void* self) {
      static_cast<TargetWithPostamble*>(self)->operator()();
    }

    static constexpr VTable vtable = {dtor, destructiveMove, invoke};

    template <typename PostF, typename TF, typename... TArgs>
    constexpr explicit TargetWithPostamble(PostF&& postF, TF&& f, TArgs&&... args)
        : PostambleStorageType(std::forward<PostF>(postF)),
          FunctionStorageType(std::forward<TF>(f)),
          ArgsStorageType(std::forward<TArgs>(args)...) {}

    constexpr void operator()() {
      std::apply(static_cast<FunctionStorageType&>(*this),
                 std::move(static_cast<ArgsStorageType&>(*this)));
      static_cast<PostambleStorageType&> (*this)();
    }
  };

  // Target with result assigned to pointer
  // Callable and arguments are stored using inheritance to allow EBO
  template <typename ResultPtr, typename F, typename... Args>
  struct TargetWithResult : detail::FunctionStorage<F>, std::tuple<Args...> {
    using FunctionStorageType = detail::FunctionStorage<F>;
    using ArgsStorageType = std::tuple<Args...>;

    static void dtor(void* self) noexcept {
      static_cast<TargetWithResult*>(self)->~TargetWithResult();
    }

    static void destructiveMove(void* self, void* p) {
      new (p) TargetWithResult(std::move(*static_cast<TargetWithResult*>(self)));
      static_cast<TargetWithResult*>(self)->~TargetWithResult();
    }

    static void invoke(void* self) {
      static_cast<TargetWithResult*>(self)->operator()();
    }

    static constexpr VTable vtable = {dtor, destructiveMove, invoke};

    ResultPtr resultPtr;

    template <typename TResultPtr, typename TF, typename... TArgs,
              std::enable_if_t<!std::is_same_v<std::decay_t<TF>, TargetWithResult>, int> = 0>
    constexpr TargetWithResult(TResultPtr&& resultPtr, TF&& f, TArgs&&... args)
        : FunctionStorageType(std::forward<TF>(f)),
          ArgsStorageType(std::forward<TArgs>(args)...),
          resultPtr(std::forward<TResultPtr>(resultPtr)) {}

    constexpr void operator()() {
      using ResultType = typename std::pointer_traits<ResultPtr>::element_type;
      using ValueType = typename ResultType::ValueType;

      if constexpr (std::is_same_v<ValueType, void>) {
        std::apply(static_cast<FunctionStorageType&>(*this),
                   std::move(static_cast<ArgsStorageType&>(*this)));
      } else {
        auto dataPtr = memberPtrOf<std::byte>(resultPtr, offsetof(ResultType, data));
        auto ptr = globalPtrReinterpretCast<GlobalPtr<ValueType>>(dataPtr);
        constructAt(ptr, std::apply(static_cast<FunctionStorageType&>(*this),
                                    std::move(static_cast<ArgsStorageType&>(*this))));
      }
      // make sure that result is visible since it is consumed by another thread
      auto readyPtr = memberPtrOf<std::int32_t>(resultPtr, offsetof(ResultType, ready));
      atomicStore(readyPtr, 1, std::memory_order_release);
      if (!isOnCP()) {
        TerminationDetection::increaseTasksFinished(1);
      }
    }
  };

  // Indirect target
  template <typename F>
  struct IndirectTarget {
    static void dtor(void* self) noexcept {
      static_cast<IndirectTarget*>(self)->~IndirectTarget();
    }

    static void destructiveMove(void* self, void* p) {
      new (p) IndirectTarget(std::move(*static_cast<IndirectTarget*>(self)));
      static_cast<IndirectTarget*>(self)->~IndirectTarget();
    }

    static void invoke(void* self) {
      static_cast<IndirectTarget*>(self)->operator()();
    }

    static constexpr VTable vtable = {dtor, destructiveMove, invoke};

    F* fPtr{};

    template <typename TF, typename... TArgs,
              std::enable_if_t<!std::is_same_v<std::decay_t<TF>, IndirectTarget>, int> = 0>
    explicit IndirectTarget(TF&& f, TArgs&&... args)
        : fPtr(new F(std::forward<TF>(f), std::forward<TArgs>(args)...)) {}

    IndirectTarget(const IndirectTarget&) = delete;

    constexpr IndirectTarget(IndirectTarget&& other) noexcept
        : fPtr(std::exchange(other.fPtr, nullptr)) {}

    ~IndirectTarget() {
      delete fPtr;
    }

    IndirectTarget& operator=(const IndirectTarget&) = delete;
    IndirectTarget& operator=(IndirectTarget&&) = delete;

    constexpr void operator()() {
      (*fPtr)();
    }
  };

  // Empty target
  struct EmptyTarget {
    static void dtor(void*) noexcept {} // NOLINT(readability/casting) unnamed arg
    static void destructiveMove(void*, void*) {}

    static constexpr VTable vtable = {dtor, destructiveMove, nullptr};
  };

  // storage for the target
  alignas(MaxAlignT) std::byte m_storage[SmallObjectStorageSize];
  // vtable to manipulate m_storage as the right type
  const VTable* m_vtable = &EmptyTarget::vtable;

  static_assert(sizeof(m_storage) >= sizeof(void (*)()),
                "Insufficient storage for function pointers");

public:
  Task() noexcept = default;

  /**
   * @brief Constructs a @ref Task object from a function @p f and arguments @p args.
   *
   * @param[in] f    function to invoke
   * @param[in] args arguments to pass to @p f
   */
  template <typename F, typename... Args,
            std::enable_if_t<!std::is_same_v<std::decay_t<F>, Task>, int> = 0>
  explicit constexpr Task(F&& f, Args&&... args) {
    using TargetType = Target<std::decay_t<F>, std::decay_t<Args>...>;

    // use small object optimization iff alignment and size are satisfied for the stored target
    constexpr bool IsSmallObject = (sizeof(TargetType) <= sizeof(m_storage)) &&
                                   (alignof(TargetType) <= alignof(std::max_align_t));
    using TargetImpl = std::conditional_t<IsSmallObject, TargetType, IndirectTarget<TargetType>>;

    new (m_storage) TargetImpl(std::forward<F>(f), std::forward<Args>(args)...);
    m_vtable = &TargetImpl::vtable;
  }

  /**
   * @brief Constructs a @ref Task object from a function @p f and arguments @p args with a
   *        postamble.
   *
   * @param[in] postamble function to invoke after @p f
   * @param[in] f         function to invoke
   * @param[in] args      arguments to pass to @p f
   */
  template <typename Postamble, typename F, typename... Args>
  explicit constexpr Task(WithPostamble, Postamble&& postamble, F&& f, Args&&... args) {
    using TargetType =
        TargetWithPostamble<std::decay_t<Postamble>, std::decay_t<F>, std::decay_t<Args>...>;

    // use small object optimization iff alignment and size are satisfied for the stored target
    constexpr bool IsSmallObject = (sizeof(TargetType) <= sizeof(m_storage)) &&
                                   (alignof(TargetType) <= alignof(std::max_align_t));
    using TargetImpl = std::conditional_t<IsSmallObject, TargetType, IndirectTarget<TargetType>>;

    new (m_storage) TargetImpl(std::forward<Postamble>(postamble), std::forward<F>(f),
                               std::forward<Args>(args)...);
    m_vtable = &TargetImpl::vtable;
  }

  /**
   * @brief Constructs a @ref Task object from a function @p f and arguments @p args that assigns
   *        the result to `*resultPtr`.
   *
   * @param[in] resultPtr pointer to store the result of `f(args...)`
   * @param[in] f         function to invoke
   * @param[in] args      arguments to pass to @p f
   */
  template <typename ResultPtr, typename F, typename... Args>
  constexpr Task(WithResultPtr, ResultPtr resultPtr, F&& f, Args&&... args) {
    using TargetType = TargetWithResult<ResultPtr, std::decay_t<F>, std::decay_t<Args>...>;

    // use small object optimization iff alignment and size are satisfied for the stored target
    constexpr bool IsSmallObject = (sizeof(TargetType) <= sizeof(m_storage)) &&
                                   (alignof(TargetType) <= alignof(std::max_align_t));
    using TargetImpl = std::conditional_t<IsSmallObject, TargetType, IndirectTarget<TargetType>>;

    new (m_storage) TargetImpl(resultPtr, std::forward<F>(f), std::forward<Args>(args)...);
    m_vtable = &TargetImpl::vtable;
  }

  Task(const Task&) = delete;

  Task(Task&& other) noexcept : m_vtable(std::exchange(other.m_vtable, &EmptyTarget::vtable)) {
    m_vtable->destructiveMove(other.m_storage, m_storage);
  }

  ~Task() {
    m_vtable->dtor(m_storage);
  }

  Task& operator=(const Task&) = delete;

  Task& operator=(Task&& other) noexcept {
    if (this != &other) {
      m_vtable->dtor(m_storage);
      m_vtable = &EmptyTarget::vtable;
      other.m_vtable->destructiveMove(other.m_storage, m_storage);
      std::swap(m_vtable, other.m_vtable);
    }
    return *this;
  }

  constexpr void operator()() noexcept(noexcept(m_vtable->invoke(m_storage))) {
    m_vtable->invoke(m_storage);
  }
};

} // namespace pando

#endif //  PANDO_RT_EXECUTION_TASK_HPP_
