// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_UTILITY_TUPLE_HPP_
#define PANDO_LIB_GALOIS_UTILITY_TUPLE_HPP_

#include <cstdint>
#include <type_traits>

#include <pando-lib-galois/utility/pair.hpp>

#define GENVALUE(num) T##num v##num

#define GENGET(num)           \
  if constexpr (index == num) \
  return v##num

namespace galois {
template <typename T0>
struct Tuple1 {
  static constexpr std::uint64_t size = 1;
  using myTuple = Tuple1<T0>;
  T0 v0;

  template <std::uint64_t index>
  auto get() {
    static_assert(index < size);
    GENGET(0);
  }

  template <std::uint64_t index>
  friend auto get(myTuple& tpl) {
    tpl.template get<index>();
  }
};

template <typename A0>
Tuple1<A0> make_tpl(A0 a0) {
  return Tuple1<A0>{a0};
}

template <typename T0, typename T1>
using Tuple2 = galois::Pair<T0, T1>;

template <typename A0, typename A1>
Tuple2<A0, A1> make_tpl(A0 a0, A1 a1) {
  return Tuple2<A0, A1>{a0, a1};
}

template <typename T0, typename T1, typename T2>
struct Tuple3 {
  static constexpr std::uint64_t size = 3;
  using myTuple = Tuple3<T0, T1, T2>;
  T0 v0;
  T1 v1;
  T2 v2;

  template <std::uint64_t index>
  auto get() {
    static_assert(index < size);
    GENGET(0);
    GENGET(1);
    GENGET(2);
  }

  template <std::uint64_t index>
  friend auto get(myTuple& tpl) {
    tpl.template get<index>();
  }
};

template <typename A0, typename A1, typename A2>
Tuple3<A0, A1, A2> make_tpl(A0 a0, A1 a1, A2 a2) {
  return Tuple3<A0, A1, A2>{a0, a1, a2};
}

template <typename T0, typename T1, typename T2, typename T3>
struct Tuple4 {
  static constexpr std::uint64_t size = 4;
  using myTuple = Tuple4<T0, T1, T2, T3>;
  T0 v0;
  T1 v1;
  T2 v2;
  T3 v3;

  template <std::uint64_t index>
  auto get() {
    static_assert(index < size);
    GENGET(0);
    GENGET(1);
    GENGET(2);
    GENGET(3);
  }

  template <std::uint64_t index>
  friend auto get(myTuple& tpl) {
    tpl.template get<index>();
  }
};

template <typename A0, typename A1, typename A2, typename A3>
Tuple4<A0, A1, A2, A3> make_tpl(A0 a0, A1 a1, A2 a2, A3 a3) {
  return Tuple4<A0, A1, A2, A3>{a0, a1, a2, a3};
}

template <typename T0, typename T1, typename T2, typename T3, typename T4>
struct Tuple5 {
  static constexpr std::uint64_t size = 5;
  using myTuple = Tuple5<T0, T1, T2, T3, T4>;
  T0 v0;
  T1 v1;
  T2 v2;
  T3 v3;
  T4 v4;

  template <std::uint64_t index>
  auto get() {
    static_assert(index < size);
    GENGET(0);
    GENGET(1);
    GENGET(2);
    GENGET(3);
    GENGET(4);
  }

  template <std::uint64_t index>
  friend auto get(myTuple& tpl) {
    tpl.template get<index>();
  }
};

template <typename A0, typename A1, typename A2, typename A3, typename A4>
Tuple5<A0, A1, A2, A3, A4> make_tpl(A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
  return Tuple5<A0, A1, A2, A3, A4>{a0, a1, a2, a3, a4};
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
struct Tuple6 {
  static constexpr std::uint64_t size = 6;
  using myTuple = Tuple6<T0, T1, T2, T3, T4, T5>;
  T0 v0;
  T1 v1;
  T2 v2;
  T3 v3;
  T4 v4;
  T5 v5;

  template <std::uint64_t index>
  auto get() {
    static_assert(index < size);
    GENGET(0);
    GENGET(1);
    GENGET(2);
    GENGET(3);
    GENGET(4);
    GENGET(5);
  }

  template <std::uint64_t index>
  friend auto get(myTuple& tpl) {
    tpl.template get<index>();
  }
};

template <typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
Tuple6<A0, A1, A2, A3, A4, A5> make_tpl(A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) {
  return Tuple6<A0, A1, A2, A3, A4, A5>{a0, a1, a2, a3, a4, a5};
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
struct Tuple7 {
  static constexpr std::uint64_t size = 7;
  using myTuple = Tuple7<T0, T1, T2, T3, T4, T5, T6>;
  T0 v0;
  T1 v1;
  T2 v2;
  T3 v3;
  T4 v4;
  T5 v5;
  T6 v6;

  template <std::uint64_t index>
  auto get() {
    static_assert(index < size);
    GENGET(0);
    GENGET(1);
    GENGET(2);
    GENGET(3);
    GENGET(4);
    GENGET(5);
    GENGET(6);
  }

  template <std::uint64_t index>
  friend auto get(myTuple& tpl) {
    tpl.template get<index>();
  }
};

template <typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
Tuple7<A0, A1, A2, A3, A4, A5, A6> make_tpl(A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) {
  return Tuple7<A0, A1, A2, A3, A4, A5, A6>{a0, a1, a2, a3, a4, a5, a6};
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
struct Tuple8 {
  static constexpr std::uint64_t size = 8;
  using myTuple = Tuple8<T0, T1, T2, T3, T4, T5, T6, T7>;
  T0 v0;
  T1 v1;
  T2 v2;
  T3 v3;
  T4 v4;
  T5 v5;
  T6 v6;
  T7 v7;

  template <std::uint64_t index>
  auto get() {
    static_assert(index < size);
    GENGET(0);
    GENGET(1);
    GENGET(2);
    GENGET(3);
    GENGET(4);
    GENGET(5);
    GENGET(6);
    GENGET(7);
  }

  template <std::uint64_t index>
  friend auto get(myTuple& tpl) {
    tpl.template get<index>();
  }
};

template <typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
Tuple8<A0, A1, A2, A3, A4, A5, A6, A7> make_tpl(A0 a0, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) {
  return Tuple8<A0, A1, A2, A3, A4, A5, A6, A7>{a0, a1, a2, a3, a4, a5, a6, a7};
}
}; // namespace galois

namespace std {
template <std::uint64_t index, typename T0>
auto get(galois::Tuple1<T0> tpl) {
  return tpl.template get<index>();
}

template <std::uint64_t index, typename T0, typename T1>
auto get(galois::Tuple2<T0, T1> tpl) {
  return tpl.template get<index>();
}

template <std::uint64_t index, typename T0, typename T1, typename T2>
auto get(galois::Tuple3<T0, T1, T2> tpl) {
  return tpl.template get<index>();
}

template <std::uint64_t index, typename T0, typename T1, typename T2, typename T3>
auto get(galois::Tuple4<T0, T1, T2, T3> tpl) {
  return tpl.template get<index>();
}

template <std::uint64_t index, typename T0, typename T1, typename T2, typename T3, typename T4>
auto get(galois::Tuple5<T0, T1, T2, T3, T4> tpl) {
  return tpl.template get<index>();
}

template <std::uint64_t index, typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5>
auto get(galois::Tuple6<T0, T1, T2, T3, T4, T5> tpl) {
  return tpl.template get<index>();
}

template <std::uint64_t index, typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6>
auto get(galois::Tuple7<T0, T1, T2, T3, T4, T5, T6> tpl) {
  return tpl.template get<index>();
}

template <std::uint64_t index, typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7>
auto get(galois::Tuple8<T0, T1, T2, T3, T4, T5, T6, T7> tpl) {
  return tpl.template get<index>();
}
}; // namespace std

#endif // PANDO_LIB_GALOIS_UTILITY_TUPLE_HPP_
