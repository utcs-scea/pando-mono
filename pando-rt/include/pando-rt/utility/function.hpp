// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_UTILITY_FUNCTION_HPP_
#define PANDO_RT_UTILITY_FUNCTION_HPP_

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "../stddef.hpp"

namespace pando {

namespace detail {

/**
 * @brief Returns if @p f is null callable, i.e., cannot be invoked.
 *
 * Only function pointers can be null callable, as they can be @c nullptr.
 *
 * @ingroup ROOT
 */
template <typename F>
constexpr bool isNullCallable(const F& f) noexcept {
  if constexpr (std::is_pointer_v<std::decay_t<F>> || std::is_member_pointer_v<std::decay_t<F>>) {
    // function pointer or member function pointer
    return !f;
  } else {
    // function object or any other object
    return false;
  }
}

/**
 * @brief Returns @p f as a function object.
 *
 * @ingroup ROOT
 */
template <typename F>
constexpr decltype(auto) asFunctor(F&& f) noexcept {
  if constexpr (!std::is_member_function_pointer_v<std::decay_t<F>>) {
    return std::forward<F>(f);
  } else {
    return std::mem_fn(f);
  }
}

} // namespace detail

/// @private
template <typename>
class Function;

/**
 * @brief General purpose polymorphic function wrapper.
 *
 * This class is similar to @c std::function in that it can store, copy and invoke any
 * @c CopyConstructible @c Callable target but it avoids virtual tables and RTTI. It also supports
 * serialization so unlike @c std::function you can transfer it over the network.
 *
 * @ref Function satisfies the requirements of @c CopyConstructible and @c CopyAssignable.
 *
 * @ingroup ROOT
 */
template <typename R, typename... Args>
class Function<R(Args...)> {
public:
  using result_type = R;

private:
  // Vtable to access the type-erased target
  struct VTable {
    void (*dtor)(void*) noexcept; // NOLINT(readability/casting) unnamed arg
    void (*copy)(const void*, void*);
    void (*move)(void*, void*);
    R (*invoke)(const void*, Args...);
  };

  // Stored, type-erased target
  template <typename F, bool SmallObjectOptimization>
  class Callable;

  // Stored, type-erased target, small object specialization
  template <typename F>
  class Callable<F, true> {
  public:
    static void dtor(void* self) noexcept {
      static_cast<Callable*>(self)->~Callable();
    }

    static void copy(const void* self, void* p) {
      new (p) Callable(static_cast<const Callable*>(self)->m_f);
    }

    static void move(void* self, void* p) {
      new (p) Callable(std::move(static_cast<Callable*>(self)->m_f));
    }

    static R invoke(const void* self, Args... args) {
      return (static_cast<Callable*>(const_cast<void*>(self))->m_f)(std::forward<Args>(args)...);
    }

    static constexpr VTable vtable = {dtor, copy, move, invoke};

  private:
    F m_f;

  public:
    template <typename T>
    constexpr explicit Callable(T&& f) : m_f(std::forward<T>(f)) {}

    Callable(const Callable&) = delete;
    Callable(Callable&&) = delete;

  protected:
    ~Callable() = default;

  public:
    Callable& operator=(const Callable&) = delete;
    Callable& operator=(Callable&&) = delete;
  };

  // Stored, type-erased target, large object specialization
  template <typename F>
  class Callable<F, false> {
  public:
    static void dtor(void* self) noexcept {
      static_cast<Callable*>(self)->~Callable();
    }

    static void copy(const void* self, void* p) {
      new (p) Callable(*static_cast<const Callable*>(self)->m_p);
    }

    static void move(void* self, void* p) {
      new (p) Callable(std::move(*static_cast<Callable*>(self)));
    }

    static R invoke(const void* self, Args... args) {
      return (*static_cast<Callable*>(const_cast<void*>(self))->m_p)(std::forward<Args>(args)...);
    }

    static constexpr VTable vtable = {dtor, copy, move, invoke};

  private:
    std::unique_ptr<F> m_p;

  public:
    template <typename TF>
    explicit Callable(TF&& f) : m_p(std::make_unique<F>(std::forward<TF>(f))) {}

    Callable(const Callable&) = delete;

  protected:
    Callable(Callable&&) noexcept = default;

    ~Callable() = default;

  public:
    Callable& operator=(const Callable&) = delete;
    Callable& operator=(Callable&&) = delete;
  };

  // Empty target
  class NullCallable {
  public:
    static void dtor(void*) noexcept {} // NOLINT(readability/casting) unnamed arg
    static void copy(const void*, void*) {}
    static void move(void*, void*) {}

    static constexpr VTable vtable = {dtor, copy, move, nullptr};

    NullCallable() = default;

    NullCallable(const NullCallable&) = delete;
    NullCallable(NullCallable&&) = delete;

  protected:
    ~NullCallable() = default;

  public:
    NullCallable& operator=(const NullCallable&) = delete;
    NullCallable& operator=(NullCallable&&) = delete;
  };

  // storage for the target
  alignas(MaxAlignT) std::byte m_storage[std::max(alignof(MaxAlignT), 2 * sizeof(void*))];
  // vtable to manipulate m_storage as the right type
  const VTable* m_vtable = &NullCallable::vtable;

  static_assert(sizeof(m_storage) >= sizeof(void (*)()),
                "Insufficient storage for function pointers");

public:
  Function() noexcept = default;

  Function(std::nullptr_t) noexcept {}

  Function(const Function& other) : m_vtable(other.m_vtable) {
    m_vtable->copy(other.m_storage, m_storage);
  }

  Function(Function&& other) noexcept
      : m_vtable(std::exchange(other.m_vtable, &NullCallable::vtable)) {
    m_vtable->move(other.m_storage, m_storage);
    m_vtable->dtor(other.m_storage);
  }

  template <typename F, std::enable_if_t<!std::is_same_v<std::decay_t<F>, Function>, int> = 0>
  Function(F&& f) {
    if (detail::isNullCallable(f)) {
      return;
    }

    // to use small object optimization, alignment and size need to be satisfied for the stored
    // target
    using CallableType = std::decay_t<decltype(detail::asFunctor(std::forward<F>(f)))>;
    using SmallCallableType = Callable<CallableType, true>;
    using LargeCallableType = Callable<CallableType, false>;
    constexpr bool IsSmallObject = (sizeof(SmallCallableType) <= sizeof(m_storage)) &&
                                   (alignof(SmallCallableType) <= alignof(std::max_align_t));
    using StoredCallableType =
        std::conditional_t<IsSmallObject, SmallCallableType, LargeCallableType>;

    new (m_storage) StoredCallableType(detail::asFunctor(std::forward<F>(f)));
    m_vtable = &StoredCallableType::vtable;
  }

  ~Function() {
    m_vtable->dtor(m_storage);
  }

  Function& operator=(const Function& other) noexcept {
    if (this != &other) {
      *this = nullptr;

      other.m_vtable->copy(other.m_storage, m_storage);
      m_vtable = other.m_vtable;
    }
    return *this;
  }

  Function& operator=(Function&& other) noexcept {
    if (this != &other) {
      *this = nullptr;

      other.m_vtable->move(other.m_storage, m_storage);
      other.m_vtable->dtor(other.m_storage);
      std::swap(m_vtable, other.m_vtable);
    }
    return *this;
  }

  Function& operator=(std::nullptr_t) noexcept {
    m_vtable->dtor(m_storage);
    m_vtable = &NullCallable::vtable;
    return *this;
  }

  template <typename F, std::enable_if_t<!std::is_same_v<std::decay_t<F>, Function>, int> = 0>
  Function& operator=(F&& f) {
    return *this = Function(std::forward<F>(f));
  }

  void swap(Function& other) noexcept {
    std::swap(*this, other);
  }

  operator bool() const noexcept {
    return m_vtable->invoke != nullptr;
  }

  R operator()(Args... args) const {
    return m_vtable->invoke(m_storage, std::forward<Args>(args)...);
  }
};

/// @ingroup ROOT
template <typename R, typename... Args>
bool operator==(std::nullptr_t, const Function<R(Args...)>& y) noexcept {
  return !static_cast<bool>(y);
}

/// @ingroup ROOT
template <typename R, typename... Args>
bool operator==(const Function<R(Args...)>& x, std::nullptr_t) noexcept {
  return !static_cast<bool>(x);
}

/// @ingroup ROOT
template <typename R, typename... Args>
bool operator!=(std::nullptr_t, const Function<R(Args...)>& y) noexcept {
  return static_cast<bool>(y);
}

/// @ingroup ROOT
template <typename R, typename... Args>
bool operator!=(const Function<R(Args...)>& x, std::nullptr_t) noexcept {
  return static_cast<bool>(x);
}

/**
 * @brief Overloads @ref std::swap() for @ref Function<R(Args...)>.
 *
 * @ingroup ROOT
 */
template <typename R, typename... Args>
void swap(Function<R(Args...)>& x, Function<R(Args...)>& y) noexcept {
  x.swap(y);
}

} // namespace pando

#endif //  PANDO_RT_UTILITY_FUNCTION_HPP_
