// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#ifndef PANDO_RT_INDEX_HPP_
#define PANDO_RT_INDEX_HPP_

#include <cstdint>
#include <tuple>

namespace pando {

/**
 * @brief Node index type.
 *
 * This struct gives the position of a node in the system.
 *
 * @ingroup ROOT
 */
struct NodeIndex {
  std::int16_t id{};

  constexpr NodeIndex() noexcept = default;

  constexpr explicit NodeIndex(std::int16_t id) noexcept : id{id} {}
};

/**
 * @brief Special node index to tell the runtime itself to pick a node.
 *
 * @ingroup ROOT
 */
constexpr inline NodeIndex anyNode = NodeIndex{-1};

/// @ingroup ROOT
constexpr bool operator==(NodeIndex x, NodeIndex y) noexcept {
  return x.id == y.id;
}

/// @ingroup ROOT
constexpr bool operator!=(NodeIndex x, NodeIndex y) noexcept {
  return !(x == y);
}

/// @ingroup ROOT
constexpr bool operator<(NodeIndex x, NodeIndex y) noexcept {
  return x.id < y.id;
}

/// @ingroup ROOT
constexpr bool operator<=(NodeIndex x, NodeIndex y) noexcept {
  return !(y < x);
}

/// @ingroup ROOT
constexpr bool operator>(NodeIndex x, NodeIndex y) noexcept {
  return y < x;
}

/// @ingroup ROOT
constexpr bool operator>=(NodeIndex x, NodeIndex y) noexcept {
  return !(x < y);
}

/**
 * @brief Returns if the Node described by @p x are a subset of the cores described by @p y.
 *
 * @param[in] x the set that should be smaller for true
 * @param[in] y the set that should be larger or equal for true
 *
 * @return @c true if \f$x \subseteq y\f$
 *
 * @ingroup ROOT
 */
constexpr bool isSubsetOf(NodeIndex x, NodeIndex y) noexcept {
  return y == anyNode ? true : x == y;
}

/**
 * @brief Pod index type.
 *
 * This struct gives the position of a pod in a node.
 *
 * @ingroup ROOT
 */
struct PodIndex {
  std::int8_t x{};
  std::int8_t y{};

  constexpr PodIndex() noexcept = default;

  constexpr PodIndex(std::int8_t x, std::int8_t y) noexcept : x{x}, y{y} {}
};

/**
 * @brief Special pod index to tell the runtime itself to pick a pod.
 */
constexpr inline PodIndex anyPod = PodIndex{-1, -1};

/// @ingroup ROOT
constexpr bool operator==(PodIndex x, PodIndex y) noexcept {
  return (x.x == y.x) && (x.y == y.y);
}

/// @ingroup ROOT
constexpr bool operator!=(PodIndex x, PodIndex y) noexcept {
  return !(x == y);
}

/// @ingroup ROOT
constexpr bool operator<(PodIndex x, PodIndex y) noexcept {
  return (x.x < y.x) || (!(y.x < x.x) && x.y < y.y);
}

/// @ingroup ROOT
constexpr bool operator<=(PodIndex x, PodIndex y) noexcept {
  return !(y < x);
}

/// @ingroup ROOT
constexpr bool operator>(PodIndex x, PodIndex y) noexcept {
  return y < x;
}

/// @ingroup ROOT
constexpr bool operator>=(PodIndex x, PodIndex y) noexcept {
  return !(x < y);
}

/**
 * @brief Returns if the pods described by @p x are a subset of the cores described by @p y.
 *
 * @param[in] x the set that should be smaller for true
 * @param[in] y the set that should be larger or equal for true
 *
 * @return @c true if \f$x \subseteq y\f$
 *
 * @ingroup ROOT
 */
constexpr bool isSubsetOf(PodIndex x, PodIndex y) noexcept {
  return y == anyPod ? true : x == y;
}

/**
 * @brief Core index type.
 *
 * This struct gives the position of a core in a pod.
 *
 * @ingroup ROOT
 */
struct CoreIndex {
  std::int8_t x{};
  std::int8_t y{};

  constexpr CoreIndex() noexcept = default;

  constexpr CoreIndex(std::int8_t x, std::int8_t y) noexcept : x{x}, y{y} {}
};

/**
 * @brief Special core index to tell the runtime itself to pick a core.
 *
 * @ingroup ROOT
 */
constexpr inline CoreIndex anyCore = CoreIndex{-1, -1};

/// @ingroup ROOT
constexpr bool operator==(CoreIndex x, CoreIndex y) noexcept {
  return (x.x == y.x) && (x.y == y.y);
}

/// @ingroup ROOT
constexpr bool operator!=(CoreIndex x, CoreIndex y) noexcept {
  return !(x == y);
}

/// @ingroup ROOT
constexpr bool operator<(CoreIndex x, CoreIndex y) noexcept {
  return (x.x < y.x) || (!(y.x < x.x) && (x.y < y.y));
}

/// @ingroup ROOT
constexpr bool operator<=(CoreIndex x, CoreIndex y) noexcept {
  return !(y < x);
}

/// @ingroup ROOT
constexpr bool operator>(CoreIndex x, CoreIndex y) noexcept {
  return y < x;
}

/// @ingroup ROOT
constexpr bool operator>=(CoreIndex x, CoreIndex y) noexcept {
  return !(x < y);
}

/**
 * @brief Returns if the cores described by @p x are a subset of the cores described by @p y.
 *
 * @param[in] x the set that should be smaller for true
 * @param[in] y the set that should be larger or equal for true
 *
 * @return @c true if \f$x \subseteq y\f$
 *
 * @ingroup ROOT
 */
constexpr bool isSubsetOf(CoreIndex x, CoreIndex y) noexcept {
  return y == anyCore ? true : x == y;
}

/**
 * @brief Place index type.
 *
 * This struct gives the position of a core in the system.
 *
 * @ingroup ROOT
 */
struct Place {
  NodeIndex node;
  PodIndex pod;
  CoreIndex core;

  constexpr Place() noexcept = default;

  constexpr Place(NodeIndex nodeIdx, PodIndex podIdx, CoreIndex coreIdx) noexcept
      : node{nodeIdx}, pod{podIdx}, core{coreIdx} {}
};

/**
 * @brief Special place to tell the runtime itself to pick a place.
 *
 * @ingroup ROOT
 */
constexpr inline Place anyPlace = Place{anyNode, anyPod, anyCore};

/// @ingroup ROOT
constexpr bool operator==(Place x, Place y) noexcept {
  return std::tie(x.node, x.pod, x.core) == std::tie(y.node, y.pod, y.core);
}

/// @ingroup ROOT
constexpr bool operator!=(Place x, Place y) noexcept {
  return !(x == y);
}

/// @ingroup ROOT
constexpr bool operator<(Place x, Place y) noexcept {
  return std::tie(x.node, x.pod, x.core) < std::tie(y.node, y.pod, y.core);
}

/// @ingroup ROOT
constexpr bool operator<=(Place x, Place y) noexcept {
  return !(y < x);
}

/// @ingroup ROOT
constexpr bool operator>(Place x, Place y) noexcept {
  return y < x;
}

/// @ingroup ROOT
constexpr bool operator>=(Place x, Place y) noexcept {
  return !(x < y);
}

/**
 * @brief Returns if the place described by @p x are a subset of the cores described by @p y.
 *
 * @param[in] x the set that should be smaller for true
 * @param[in] y the set that should be larger or equal for true
 *
 * @return @c true if \f$x \subseteq y\f$
 *
 * @ingroup ROOT
 */
constexpr bool isSubsetOf(Place x, Place y) noexcept {
  return isSubsetOf(x.node, y.node) && isSubsetOf(x.pod, y.pod) && isSubsetOf(x.core, y.core);
}

/**
 * @brief Thread index type.
 *
 * This struct gives the thread ID of a hart in a core.
 *
 * @ingroup ROOT
 */
struct ThreadIndex {
  std::int8_t id{};

  constexpr ThreadIndex() noexcept = default;

  constexpr explicit ThreadIndex(std::int8_t id) noexcept : id{id} {}
};

/// @ingroup ROOT
constexpr bool operator==(ThreadIndex x, ThreadIndex y) noexcept {
  return (x.id == y.id);
}

/// @ingroup ROOT
constexpr bool operator!=(ThreadIndex x, ThreadIndex y) noexcept {
  return !(x == y);
}

/// @ingroup ROOT
constexpr bool operator<(ThreadIndex x, ThreadIndex y) noexcept {
  return (x.id < y.id);
}

/// @ingroup ROOT
constexpr bool operator<=(ThreadIndex x, ThreadIndex y) noexcept {
  return !(y < x);
}

/// @ingroup ROOT
constexpr bool operator>(ThreadIndex x, ThreadIndex y) noexcept {
  return y < x;
}

/// @ingroup ROOT
constexpr bool operator>=(ThreadIndex x, ThreadIndex y) noexcept {
  return !(x < y);
}

} // namespace pando

#endif //  PANDO_RT_INDEX_HPP_
