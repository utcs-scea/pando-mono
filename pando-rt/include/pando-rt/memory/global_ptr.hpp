// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_MEMORY_GLOBAL_PTR_HPP_
#define PANDO_RT_MEMORY_GLOBAL_PTR_HPP_

#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include "address_translation.hpp"
#include "export.h"
#include "global_ptr_fwd.hpp"
#ifdef PANDO_RT_USE_BACKEND_DRVX
#include "DrvAPIMemory.hpp"
#include "../drv_info.hpp"
#endif // PANDO_RT_USE_BACKEND_DRVX

namespace pando {

namespace detail {

/**
 * @brief Performs a load from a pointer in the global address space to space in the native address
 *        space.
 *
 * @param[in]  globalAddr global address to read from
 * @param[in]  n          size in bytes to read
 * @param[out] nativePtr  native pointer to write to
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT void load(GlobalAddress globalAddr, std::size_t n, void* nativePtr);

/**
 * @brief Performs a store from a pointer to the native address space to space in the global address
 *        space.
 *
 * @param[out] globalAddr global address to write to
 * @param[in]  n          size in bytes to write
 * @param[in]  nativePtr  native pointer to read from
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT void store(GlobalAddress globalAddr, std::size_t n, const void* nativePtr);

#if defined(PANDO_RT_USE_BACKEND_DRVX)

/**
 * @brief Performs a load from a pointer in the global address space to space in the native address
 *        space.
 *
 * @param[in]  globalAddr global address to read from
 * @param[out] nativePtr  native pointer to write to
 *
 * @ingroup DRVX
 */
template <typename T>
void load(GlobalAddress globalAddr, void* nativePtr) {
  T* destPtr = static_cast<T*>(nativePtr);
  *destPtr = DrvAPI::read<T>(globalAddr, DrvAPI::program_stage);
}

/**
 * @brief Performs a store from a pointer to the native address space to space in the global address
 *        space.
 *
 * @param[out] globalAddr global address to write to
 * @param[in]  nativePtr  native pointer to read from
 *
 * @ingroup DRVX
 */
template <typename T>
void store(GlobalAddress globalAddr, const void* nativePtr) {
  const T* srcPtr = static_cast<const T*>(nativePtr);
  DrvAPI::write<std::remove_cv_t<T>>(globalAddr, *srcPtr, DrvAPI::program_stage);
}

#endif
/**
 * @brief Returns if conversion is possible between two pointers of @p T and @p U.
 *
 * @ingroup ROOT
 */
template <typename From, typename To>
inline constexpr bool IsConvertibleAsPtr =
    !std::is_same_v<From, To> && std::is_convertible_v<From*, To*>;

/**
 * @brief Returns if conversion is possible between two pointers of @p T and @p U in either
 *        direction.
 *
 * @ingroup ROOT
 */
template <typename T, typename U>
inline constexpr bool AreComparableAsPtr =
    std::is_convertible_v<T*, U*> || std::is_convertible_v<U*, T*>;

/**
 * @brief Creates a global pointer from a native pointer.
 *
 * @warning The pointer @p nativePtr must belong to host memory that is a known memory.
 *
 * @param[in] nativePtr native pointer
 *
 * @return global pointer that encodes @p nativePtr to the global address pointer space or @c
 *         nullptr if the encoding could not be done
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT GlobalAddress createGlobalAddress(void* nativePtr) noexcept;

/**
 * @brief Converts a @ref GlobalAddress to a native pointer.
 *
 * @warning This function will return a valid pointer only if the pointer points to an object that
 *          is in the calling host's memory.
 *
 * @return native pointer that corresponds to @p globalAddr
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT void* asNativePtr(GlobalAddress globalAddr) noexcept;

/**
 * @brief Converts a @ref GlobalPtr to a native pointer.
 *
 * @warning This function will return a valid pointer only if the pointer points to an object that
 *          is in the calling host's memory.
 *
 * @return native pointer that corresponds to @p globalPtr, of @c nullptr if resolution could not
 *         happen
 *
 * @ingroup ROOT
 */
template <typename T>
T* asNativePtr(GlobalPtr<T> globalPtr) noexcept {
  return static_cast<T*>(asNativePtr(globalPtr.address));
}

} // namespace detail

/**
 * @brief Pointer to PANDO global address space.
 *
 * The PANDO global address space is not the same as the native address space during emulation.
 * This class can be used where a pointer is needed with most of the functionality of a regular
 * pointer.
 *
 * Unlike smart pointers, it behaves as a regular pointer. For example, an uninitialized
 * @ref GlobalPtr object will have an uninitialized pointer, leading to undefined behavior upon
 * dereference.
 *
 * Similarly, incrementing or decrementing a @ref GlobalPtr outside a known memory is
 * acceptable but dereferencing it is undefined behavior.
 *
 * @ingroup ROOT
 */
template <typename T>
struct GlobalPtr {
  GlobalAddress address;

  GlobalPtr() noexcept = default;

  constexpr GlobalPtr(std::nullptr_t) noexcept // NOLINT - implicit conversion allowed
      : address{0} {}

  /**
   * @brief Creates a new @ref GlobalPtr using @p ptr.
   *
   * @warning The pointer @p ptr must belong to host memory that is a known memory, otherwise
   *          this function will result in undefined behavior.
   *
   * @param[in] ptr pointer to create the global pointer from
   */
  GlobalPtr(T* ptr); // NOLINT - implicit conversion allowed

  constexpr GlobalPtr(const GlobalPtr&) noexcept = default;
  constexpr GlobalPtr(GlobalPtr&&) noexcept = default;

  // construction from convertible types

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, T>, int> = 0>
  constexpr GlobalPtr(const GlobalPtr<U>& other) noexcept : address(other.address) {}

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, T>, int> = 0>
  constexpr GlobalPtr(GlobalPtr<U>&& other) noexcept : address(other.address) {}

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, T>, int> = 0>
  constexpr GlobalPtr(GlobalPtr<U>& other) noexcept : address(other.address) {}

  ~GlobalPtr() = default;

  constexpr GlobalPtr& operator=(const GlobalPtr&) noexcept = default;
  constexpr GlobalPtr& operator=(GlobalPtr&&) noexcept = default;

  // assignment from convertible types

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, T>, int> = 0>
  constexpr GlobalPtr& operator=(const GlobalPtr<U>& other) noexcept {
    address = other.address;
    return *this;
  }

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, T>, int> = 0>
  constexpr GlobalPtr& operator=(GlobalPtr<U>&& other) noexcept {
    address = other.address;
    return *this;
  }

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, T>, int> = 0>
  constexpr GlobalPtr& operator=(GlobalPtr<U>& other) noexcept {
    address = other.address;
    return *this;
  }

  constexpr GlobalPtr& operator=(std::nullptr_t) noexcept {
    address = 0;
    return *this;
  }

  constexpr explicit operator bool() const noexcept {
    return address != 0;
  }

  /**
   * @brief Explicit conversion operator to support @c static_cast to convertible types.
   */
  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, T>, int> = 0>
  constexpr explicit operator GlobalPtr<U>() const noexcept {
    GlobalPtr<U> ptr;
    ptr.address = address;
    return ptr;
  }

  template <typename Size>
  constexpr auto operator[](Size n) const noexcept {
    return GlobalRef<T>{*this + n};
  }

  constexpr auto operator*() const noexcept {
    return GlobalRef<T>{*this};
  }

  GlobalPtr<GlobalPtr<T>> operator&() const noexcept { // NOLINT - needed to get a global pointer
    return GlobalPtr<GlobalPtr<T>>(this);
  }

  T* operator->() const noexcept {
    return detail::asNativePtr(*this);
  }

  constexpr GlobalPtr operator+() const noexcept {
    return *this;
  }

  // addition operators

  constexpr GlobalPtr& operator++() noexcept {
    address += sizeof(T);
    return *this;
  }

  constexpr GlobalPtr operator++(int) noexcept {
    auto copy = *this;
    address += sizeof(T);
    return copy;
  }

  template <typename Size>
  constexpr GlobalPtr& operator+=(Size n) noexcept {
    address += n * sizeof(T);
    return *this;
  }

  // subtraction operators

  constexpr GlobalPtr& operator--() noexcept {
    address -= sizeof(T);
    return *this;
  }

  constexpr GlobalPtr operator--(int) noexcept {
    auto copy = *this;
    address -= sizeof(T);
    return copy;
  }

  template <typename Size>
  constexpr GlobalPtr& operator-=(Size n) noexcept {
    address -= n * sizeof(T);
    return *this;
  }
};

/**
 * @brief Specialization of @ref GlobalPtr for @c void* global pointers.
 *
 * @ingroup ROOT
 */
template <>
struct GlobalPtr<void> {
  GlobalAddress address;

  GlobalPtr() noexcept = default;

  constexpr GlobalPtr(std::nullptr_t) noexcept // NOLINT - implicit conversion allowed
      : address{0} {}

  /// @copydoc GlobalPtr<T>::GlobalPtr(T*)
  GlobalPtr(void* ptr); // NOLINT - implicit conversion allowed

  constexpr GlobalPtr(const GlobalPtr&) noexcept = default;
  constexpr GlobalPtr(GlobalPtr&&) noexcept = default;

  // construction from convertible types

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, void>, int> = 0>
  constexpr GlobalPtr(const GlobalPtr<U>& other) noexcept : address(other.address) {}

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, void>, int> = 0>
  constexpr GlobalPtr(GlobalPtr<U>&& other) noexcept : address(other.address) {}

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, void>, int> = 0>
  constexpr GlobalPtr(GlobalPtr<U>& other) noexcept : address(other.address) {}

  ~GlobalPtr() = default;

  constexpr GlobalPtr& operator=(const GlobalPtr&) noexcept = default;
  constexpr GlobalPtr& operator=(GlobalPtr&&) noexcept = default;

  // assignment from convertible types

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, void>, int> = 0>
  constexpr GlobalPtr& operator=(const GlobalPtr<U>& other) noexcept {
    address = other.address;
    return *this;
  }

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, void>, int> = 0>
  constexpr GlobalPtr& operator=(GlobalPtr<U>&& other) noexcept {
    address = other.address;
    return *this;
  }

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, void>, int> = 0>
  constexpr GlobalPtr& operator=(GlobalPtr<U>& other) noexcept {
    address = other.address;
    return *this;
  }

  constexpr GlobalPtr& operator=(std::nullptr_t) noexcept {
    address = 0;
    return *this;
  }

  constexpr explicit operator bool() const noexcept {
    return address != 0;
  }

  /**
   * @brief Explicit conversion operator to support @c static_cast to any type.
   */
  template <typename U>
  constexpr explicit operator GlobalPtr<U>() const noexcept {
    GlobalPtr<U> ptr;
    ptr.address = address;
    return ptr;
  }
};

/**
 * @brief Specialization of @ref GlobalPtr for `const void*` global pointers.
 *
 * @ingroup ROOT
 */
template <>
struct GlobalPtr<const void> {
  GlobalAddress address;

  GlobalPtr() noexcept = default;

  constexpr GlobalPtr(std::nullptr_t) noexcept // NOLINT - implicit conversion allowed
      : address{0} {}

  /// @copydoc GlobalPtr<T>::GlobalPtr(T*)
  GlobalPtr(const void* ptr); // NOLINT - implicit conversion allowed

  constexpr GlobalPtr(const GlobalPtr&) noexcept = default;
  constexpr GlobalPtr(GlobalPtr&&) noexcept = default;

  // construction from convertible types

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, const void>, int> = 0>
  constexpr GlobalPtr(const GlobalPtr<U>& other) noexcept : address(other.address) {}

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, const void>, int> = 0>
  constexpr GlobalPtr(GlobalPtr<U>&& other) noexcept : address(other.address) {}

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, const void>, int> = 0>
  constexpr GlobalPtr(GlobalPtr<U>& other) noexcept : address(other.address) {}

  ~GlobalPtr() = default;

  constexpr GlobalPtr& operator=(const GlobalPtr&) noexcept = default;
  constexpr GlobalPtr& operator=(GlobalPtr&&) noexcept = default;

  // assignment from convertible types

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, void>, int> = 0>
  constexpr GlobalPtr& operator=(const GlobalPtr<U>& other) noexcept {
    address = other.address;
    return *this;
  }

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, void>, int> = 0>
  constexpr GlobalPtr& operator=(GlobalPtr<U>&& other) noexcept {
    address = other.address;
    return *this;
  }

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, void>, int> = 0>
  constexpr GlobalPtr& operator=(GlobalPtr<U>& other) noexcept {
    address = other.address;
    return *this;
  }

  constexpr GlobalPtr& operator=(std::nullptr_t) noexcept {
    address = 0;
    return *this;
  }

  constexpr explicit operator bool() const noexcept {
    return address != 0;
  }

  /**
   * @brief Explicit conversion operator to support @c static_cast to any type.
   */
  template <typename U>
  constexpr explicit operator GlobalPtr<const U>() const noexcept {
    GlobalPtr<const U> ptr;
    ptr.address = address;
    return ptr;
  }
};

// PREP and DrvX use address translation from host native pointers to PANDO global address space
// pointers

template <typename T>
GlobalPtr<T>::GlobalPtr(T* nativePtr) // NOLINT - implicit conversion expected
    : address(detail::createGlobalAddress(const_cast<std::remove_cv_t<T>*>(nativePtr))) {}

inline GlobalPtr<void>::GlobalPtr(void* nativePtr) // NOLINT - implicit conversion expected
    : address(detail::createGlobalAddress(nativePtr)) {}

inline GlobalPtr<const void>::GlobalPtr(
    const void* nativePtr) // NOLINT - implicit conversion expected
    : address(detail::createGlobalAddress(const_cast<void*>(nativePtr))) {}

// addition operators

template <typename T, typename Size, std::enable_if_t<std::is_arithmetic_v<Size>, int> = 0>
constexpr GlobalPtr<T> operator+(GlobalPtr<T> ptr, Size n) noexcept {
  ptr += n;
  return ptr;
}

template <typename Size, typename T, std::enable_if_t<std::is_arithmetic_v<Size>, int> = 0>
constexpr GlobalPtr<T> operator+(Size n, GlobalPtr<T> ptr) noexcept {
  ptr += n;
  return ptr;
}

// subtraction operators

template <typename T, typename Size, std::enable_if_t<std::is_arithmetic_v<Size>, int> = 0>
constexpr GlobalPtr<T> operator-(GlobalPtr<T> ptr, Size n) noexcept {
  ptr -= n;
  return ptr;
}

template <typename T>
constexpr std::ptrdiff_t operator-(GlobalPtr<T> x, GlobalPtr<T> y) noexcept {
  return static_cast<std::make_signed_t<GlobalAddress>>((x.address - y.address) / sizeof(T));
}

// comparison operators

/// @ingroup ROOT
template <typename T1, typename T2, std::enable_if_t<detail::AreComparableAsPtr<T1, T2>, int> = 0>
constexpr bool operator==(GlobalPtr<T1> x, GlobalPtr<T2> y) noexcept {
  return x.address == y.address;
}

/// @ingroup ROOT
template <typename T1, typename T2, std::enable_if_t<detail::AreComparableAsPtr<T1, T2>, int> = 0>
constexpr bool operator!=(GlobalPtr<T1> x, GlobalPtr<T2> y) noexcept {
  return x.address != y.address;
}

/// @ingroup ROOT
template <typename T1, typename T2, std::enable_if_t<detail::AreComparableAsPtr<T1, T2>, int> = 0>
constexpr bool operator<(GlobalPtr<T1> x, GlobalPtr<T2> y) noexcept {
  return x.address < y.address;
}

/// @ingroup ROOT
template <typename T1, typename T2, std::enable_if_t<detail::AreComparableAsPtr<T1, T2>, int> = 0>
constexpr bool operator<=(GlobalPtr<T1> x, GlobalPtr<T2> y) noexcept {
  return !(y < x);
}

/// @ingroup ROOT
template <typename T1, typename T2, std::enable_if_t<detail::AreComparableAsPtr<T1, T2>, int> = 0>
constexpr bool operator>(GlobalPtr<T1> x, GlobalPtr<T2> y) noexcept {
  return y < x;
}

/// @ingroup ROOT
template <typename T1, typename T2, std::enable_if_t<detail::AreComparableAsPtr<T1, T2>, int> = 0>
constexpr bool operator>=(GlobalPtr<T1> x, GlobalPtr<T2> y) noexcept {
  return !(x < y);
}

// comparison operators with nullptr

/// @ingroup ROOT
template <typename T>
constexpr bool operator==(GlobalPtr<T> x, std::nullptr_t) noexcept {
  return !x;
}

/// @ingroup ROOT
template <typename T>
constexpr bool operator==(std::nullptr_t, GlobalPtr<T> y) noexcept {
  return !y;
}

/// @ingroup ROOT
template <typename T>
constexpr bool operator!=(GlobalPtr<T> x, std::nullptr_t) noexcept {
  return static_cast<bool>(x);
}

/// @ingroup ROOT
template <typename T>
constexpr bool operator!=(std::nullptr_t, GlobalPtr<T> y) noexcept {
  return static_cast<bool>(y);
}

/**
 * @brief @ref std::swap() specialization for @ref GlobalPtr.
 *
 * @ingroup ROOT
 */
template <typename T>
void swap(GlobalPtr<T>& x, GlobalPtr<T>& y) noexcept {
  using std::swap;
  swap(x.address, y.address);
}

/**
 * @brief Returns the memory type associated with a global pointer.
 */
PANDO_RT_EXPORT MemoryType memoryTypeOf(GlobalPtr<const void> ptr) noexcept;

/**
 * @brief Returns the place associated with a global pointer.
 *
 * @param[in] ptr global pointer
 *
 * @return the place @p globalPtr is closest to
 *
 * @ingroup ROOT
 */
PANDO_RT_EXPORT Place localityOf(GlobalPtr<const void> ptr) noexcept;

/**
 * @brief Checks if @p T is a @ref GlobalPtr.
 *
 * @ingroup ROOT
 */
template <typename T>
struct IsGlobalPtr : std::false_type {};

/**
 * @brief Specialization of @ref IsGlobalPtr for global pointers.
 *
 * @ingroup ROOT
 */
template <typename T>
struct IsGlobalPtr<GlobalPtr<T>> : std::true_type {};

/// @ingroup ROOT
template <typename T>
inline constexpr bool isGlobalPtrV = IsGlobalPtr<T>::value;

/**
 * @brief Reinterpret cast support for @ref GlobalPtr.
 *
 * @ingroup ROOT
 */
template <typename To, typename From>
To globalPtrReinterpretCast(From from) noexcept {
#if defined(PANDO_RT_USE_BACKEND_PREP) || defined(PANDO_RT_USE_BACKEND_DRVX)
  if constexpr (std::is_same_v<From, To>) {
    // same type
    return from;
  } else if constexpr (isGlobalPtrV<From> && std::is_integral_v<To>) {
    // ptr -> integral
    static_assert(sizeof(GlobalAddress) <= sizeof(To), "cast from global pointer to smaller type");
    return reinterpret_cast<To>(from.address);
  } else if constexpr (std::is_integral_v<From> && isGlobalPtrV<To>) {
    // integral -> ptr
    static_assert(sizeof(GlobalAddress) >= sizeof(From), "cast from larger type to global pointer");
    To toPtr;
    toPtr.address = from;
    return toPtr;
  } else if constexpr (isGlobalPtrV<From> && isGlobalPtrV<To>) {
    // ptr -> ptr
    To toPtr;
    toPtr.address = from.address;
    return toPtr;
  } else {
    static_assert((sizeof(To), false), "invalid cast");
    return To{};
  }
#else
  return reinterpret_cast<To>(from);
#endif
}

/**
 * @brief Returns a @ref GlobalPtr to the member of the object pointed to by @p ptr in offset
 *        @p offsetOfMember.
 *
 * @warning This relies on @p offsetof() and so only limited types are supported, e.g., @p T that
 *          are standard layout.
 *
 * @ingroup ROOT
 */
template <typename U, typename T>
GlobalPtr<U> memberPtrOf(GlobalPtr<T> ptr, std::size_t offsetOfMember) noexcept {
  return globalPtrReinterpretCast<GlobalPtr<U>>(
      globalPtrReinterpretCast<GlobalPtr<std::byte>>(ptr) + offsetOfMember);
}

} // namespace pando

/**
 * @brief @c std::hash<T> specialization for @ref pando::GlobalPtr.
 *
 * @ingroup ROOT
 */
template <typename T>
struct std::hash<pando::GlobalPtr<T>> {
  constexpr std::size_t operator()(pando::GlobalPtr<T> ptr) const noexcept {
    return std::hash<pando::GlobalAddress>{}(ptr.address);
  }
};

/**
 * @brief @c std::pointer_traits<T> specialization for @ref pando::GlobalPtr.
 *
 * @ingroup ROOT
 */
template <typename T>
struct std::pointer_traits<pando::GlobalPtr<T>> {
  using pointer = pando::GlobalPtr<T>;
  using element_type = T;
  using difference_type = std::ptrdiff_t;

  template <typename U>
  using rebind = pando::GlobalPtr<U>;

  static pointer pointer_to(element_type& r) {
    return static_cast<pointer>(&r);
  }
};

/**
 * @brief @c std::iterator_traits<T> specialization for @ref pando::GlobalPtr.
 *
 * @ingroup ROOT
 */
template <typename T>
struct std::iterator_traits<pando::GlobalPtr<T>> {
  using value_type = T;
  using pointer = pando::GlobalPtr<T>;
  using reference = pando::GlobalRef<T>;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::random_access_iterator_tag;
};

namespace pando {

/**
 * @brief Reference-type class that is used to access the data pointed to by a @ref GlobalPtr.
 *
 * @ingroup ROOT
 */
template <typename T>
class GlobalRef {
  GlobalPtr<T> m_globalPtr;

  template <typename>
  friend struct GlobalPtr;

  template <typename>
  friend class GlobalRef;

  /**
   * @brief Constructs a new global reference from a global pointer.
   *
   * @param[in] ptr global pointer to create the reference from
   */
  constexpr explicit GlobalRef(GlobalPtr<T> ptr) noexcept : m_globalPtr{ptr} {}

public:
  ~GlobalRef() = default;

  /**
   * @brief Copy constructor.
   *
   * Stores a reference to the object that @c other references.
   */
  constexpr GlobalRef(const GlobalRef& other) noexcept = default;

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, T>, int> = 0>
  constexpr GlobalRef(const GlobalRef<U>& other) noexcept : m_globalPtr(other.m_globalPtr) {}

  /**
   * @brief Move constructor.
   *
   * Stores a reference to the object that @c other references.
   */
  constexpr GlobalRef(GlobalRef&& other) noexcept = default;

  template <typename U, std::enable_if_t<detail::IsConvertibleAsPtr<U, T>, int> = 0>
  constexpr GlobalRef(GlobalRef<U>&& other) noexcept : m_globalPtr(other.m_globalPtr) {}

  /**
   * @brief Returns the address of the referenced object.
   */
  GlobalPtr<T> operator&() const noexcept { // NOLINT - needed to get a global pointer
    return m_globalPtr;
  }

  /**
   * @brief Returns the value of the referenced object.
   */
  operator T() const {
    static_assert(std::is_trivially_copyable_v<T>, "Only trivially copyable types are supported");

    // create storage to store the data from the global pointer
    using ObjectT = std::remove_cv_t<T>;
    alignas(ObjectT) std::byte storage[sizeof(ObjectT)];
    auto nativePtr = reinterpret_cast<ObjectT*>(&storage[0]);
#if defined(PANDO_RT_USE_BACKEND_PREP)
    detail::load(m_globalPtr.address, sizeof(ObjectT), nativePtr);
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
    if constexpr (std::is_scalar_v<ObjectT>) {
      detail::load<ObjectT>(m_globalPtr.address, nativePtr);
    } else {
      detail::load(m_globalPtr.address, sizeof(ObjectT), nativePtr);
    }
#endif
    if constexpr (std::is_trivially_destructible_v<ObjectT>) {
      // skip intermediate copy if the destructor does not do much
      return *nativePtr;
    } else {
      // need to create temporary, since destructor is not trivial
      auto tmp = std::move(*nativePtr);
      std::destroy_at(nativePtr);
      return tmp;
    }
  }

  // assignment operators

  /**
   * @brief Copy assignment operator.
   *
   * Assigns the value of @p other to this reference.
   */
  GlobalRef operator=(const GlobalRef& other) noexcept {
    if (this != std::addressof(other)) {
      operator=(static_cast<T>(other));
    }
    return *this;
  }

  /**
   * @brief Move assignment operator.
   *
   * Assigns the value of @p other to this reference.
   */
  GlobalRef operator=(GlobalRef&& other) noexcept {
    if (this != std::addressof(other)) {
      operator=(static_cast<T>(std::move(other)));
    }
    return *this;
  }

  GlobalRef operator=(const T& value) {
#if defined(PANDO_RT_USE_BACKEND_PREP)
    detail::store(m_globalPtr.address, sizeof(T), std::addressof(value));
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
    if constexpr (std::is_scalar_v<T>) {
      detail::store<T>(m_globalPtr.address, std::addressof(value));
    } else {
      detail::store(m_globalPtr.address, sizeof(T), std::addressof(value));
    }
#endif
    return *this;
  }

  template <typename U, std::enable_if_t<!std::is_same_v<std::decay_t<U>, GlobalRef> &&
                                             !std::is_same_v<std::decay_t<U>, std::decay_t<T>>,
                                         int> = 0>
  GlobalRef operator=(U&& value) {
    T tmp = std::forward<U>(value);
#if defined(PANDO_RT_USE_BACKEND_PREP)
    detail::store(m_globalPtr.address, sizeof(T), std::addressof(tmp));
#elif defined(PANDO_RT_USE_BACKEND_DRVX)
    if constexpr (std::is_scalar_v<T>) {
      detail::store<T>(m_globalPtr.address, std::addressof(tmp));
    } else {
      detail::store(m_globalPtr.address, sizeof(T), std::addressof(tmp));
    }
#endif
    return *this;
  }

  // increment / decrement operators

  GlobalRef operator++() {
    auto refValue = static_cast<T>(*this);
    *this = ++refValue;
    return *this;
  }

  T operator++(int) {
    // avoid double dereference to avoid multiple loads
    auto refValue = static_cast<T>(*this);
    auto oldValue = refValue++;
    *this = refValue;
    return oldValue;
  }

  GlobalRef operator--() {
    auto refValue = static_cast<T>(*this);
    *this = --refValue;
    return *this;
  }

  T operator--(int) {
    // avoid double dereference to avoid multiple loads
    auto refValue = static_cast<T>(*this);
    auto oldValue = refValue--;
    *this = refValue;
    return oldValue;
  }
};

// macros to generate operators

// generates assignment operators
#define PANDO_GLOBAL_REF_ASSIGN_OP(op)               \
  template <typename T, typename U>                  \
  auto operator op(GlobalRef<T> x, const U& y) {     \
    auto refValue = static_cast<T>(x);               \
    x = refValue op y;                               \
    return x;                                        \
  }                                                  \
  template <typename T, typename U>                  \
  auto operator op(GlobalRef<T> x, GlobalRef<U> y) { \
    return operator op(x, static_cast<U>(y));        \
  }

// generaters binary operators
#define PANDO_GLOBAL_REF_BIN_OP(op)                  \
  template <typename T, typename U>                  \
  auto operator op(GlobalRef<T> x, GlobalRef<U> y) { \
    return static_cast<T>(x) op static_cast<U>(y);   \
  }                                                  \
  template <typename T, typename U>                  \
  auto operator op(GlobalRef<T> x, const U& y) {     \
    return static_cast<T>(x) op y;                   \
  }                                                  \
  template <typename T, typename U>                  \
  auto operator op(const T& x, GlobalRef<U> y) {     \
    return x op static_cast<U>(y);                   \
  }                                                  \
  template <typename T, typename U>                  \
  auto operator op(GlobalRef<T> x, U& y) {     \
    return static_cast<T>(x) op y;                   \
  }                                                  \

// assignment operators

PANDO_GLOBAL_REF_ASSIGN_OP(+=)
PANDO_GLOBAL_REF_ASSIGN_OP(-=)
PANDO_GLOBAL_REF_ASSIGN_OP(*=)
PANDO_GLOBAL_REF_ASSIGN_OP(/=)
PANDO_GLOBAL_REF_ASSIGN_OP(%=)
PANDO_GLOBAL_REF_ASSIGN_OP(&=)
PANDO_GLOBAL_REF_ASSIGN_OP(|=)
PANDO_GLOBAL_REF_ASSIGN_OP(^=)
PANDO_GLOBAL_REF_ASSIGN_OP(<<=)
PANDO_GLOBAL_REF_ASSIGN_OP(>>=)

// arithmetic operators

template <typename T>
auto operator+(GlobalRef<T> ref) {
  return +static_cast<T>(ref);
}

template <typename T>
auto operator-(GlobalRef<T> ref) {
  return -static_cast<T>(ref);
}

template <typename T>
auto operator~(GlobalRef<T> ref) {
  return ~(static_cast<T>(ref));
}

PANDO_GLOBAL_REF_BIN_OP(+)
PANDO_GLOBAL_REF_BIN_OP(-)
PANDO_GLOBAL_REF_BIN_OP(*)
PANDO_GLOBAL_REF_BIN_OP(/)
PANDO_GLOBAL_REF_BIN_OP(%)
PANDO_GLOBAL_REF_BIN_OP(^)
PANDO_GLOBAL_REF_BIN_OP(&)
PANDO_GLOBAL_REF_BIN_OP(|)

// logical operators

template <typename T>
bool operator!(GlobalRef<T> ref) {
  return !static_cast<T>(ref);
}

PANDO_GLOBAL_REF_BIN_OP(&&)
PANDO_GLOBAL_REF_BIN_OP(||)

// comparison operators

PANDO_GLOBAL_REF_BIN_OP(==)
PANDO_GLOBAL_REF_BIN_OP(!=)
PANDO_GLOBAL_REF_BIN_OP(<) // NOLINT (false positive for formatting)
PANDO_GLOBAL_REF_BIN_OP(>) // NOLINT (false positive for formatting)
PANDO_GLOBAL_REF_BIN_OP(<=)
PANDO_GLOBAL_REF_BIN_OP(>=)

// undefine generator macros

#undef PANDO_GLOBAL_REF_BIN_OP
#undef PANDO_GLOBAL_REF_ASSIGN_OP

/**
 * @brief @c std::swap() specialization for @ref GlobalRef.
 *
 * @ingroup ROOT
 */
template <typename T>
void swap(GlobalRef<T> x, GlobalRef<T> y) noexcept {
  T temp = x;
  x = std::move(y);
  y = std::move(temp);
}

/**
 * @brief @c std::iter_swap() specialization for @ref GlobalPtr.
 *
 * @ingroup ROOT
 */
template <typename T>
constexpr void iter_swap(GlobalPtr<T> a, GlobalPtr<T> b) noexcept {
  swap(*a, *b);
}

} // namespace pando

#endif // PANDO_RT_MEMORY_GLOBAL_PTR_HPP_
