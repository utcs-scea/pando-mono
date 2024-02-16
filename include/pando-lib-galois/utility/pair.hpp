// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_UTILITY_PAIR_HPP_
#define PANDO_LIB_GALOIS_UTILITY_PAIR_HPP_

#include <cstdint>
#include <type_traits>

namespace galois {

#define OP_EXISTS(name, op)                                                     \
  template <typename T, typename OpTo>                                          \
  struct exists_##name {                                                        \
    template <class U, class V>                                                 \
    static auto test(U*) -> decltype(std::declval<U>() op std::declval<V>());   \
    template <class U, class V>                                                 \
    static auto test(...) -> std::false_type;                                   \
    using type = typename std::is_same<bool, decltype(test<T, OpTo>(0))>::type; \
    static const bool exists = type::value;                                     \
  }

template <typename T0, typename T1>
struct Pair {
  T0 first;
  T1 second;

  Pair() noexcept = default;

  Pair<T0, T1>(const Pair<T0, T1>&) = default;
  Pair<T0, T1>(Pair<T0, T1>&&) = default;

  ~Pair<T0, T1>() = default;

  Pair<T0, T1>& operator=(const Pair<T0, T1>&) = default;
  Pair<T0, T1>& operator=(Pair<T0, T1>&&) = default;

  template <std::uint64_t index>
  auto get() {
    static_assert(index < 2);
    if constexpr (index == 0)
      return first;
    if constexpr (index == 1)
      return second;
  }

  template <std::uint64_t index>
  friend auto get(Pair<T0, T1>& tpl) {
    return tpl.template get<index>();
  }

  OP_EXISTS(equals, ==);

  template <
      std::enable_if_t<exists_equals<T0, T0>::exists && exists_equals<T1, T1>::exists>* = nullptr>
  friend bool operator==(const Pair& a, const Pair& b) {
    return a.first == b.first && a.second == b.second;
  }

  friend bool less(const Pair& a, const Pair& b, std::true_type, std::true_type) {
    if (a.first < b.first) {
      return true;
    } else if (a.first == b.first) {
      return a.second < b.second;
    }
    return false;
  }

  friend bool less(const Pair& a, const Pair& b, std::true_type, std::false_type) {
    return a.first < b.first;
  }

  friend bool less(const Pair& a, const Pair& b, std::false_type, std::true_type) {
    return a.second < b.second;
  }

  friend bool less(const Pair& a, const Pair& b, std::false_type, std::false_type) {
    static_assert(exists_less<T0, T0>::exists && exists_less<T1, T1>::exists);
    return false;
  }

  OP_EXISTS(less, <);

  friend bool operator<(const Pair& a, const Pair& b) {
    return less(a, b,
                std::conditional_t<exists_less<T0, T0>::exists, std::true_type, std::false_type>{},
                std::conditional_t<exists_less<T1, T1>::exists, std::true_type, std::false_type>{});
  }
};
} // namespace galois
#endif // PANDO_LIB_GALOIS_UTILITY_PAIR_HPP_
