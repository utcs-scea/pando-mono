// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_CONTAINERS_HOST_LOCAL_ARRAY_HPP_
#define PANDO_LIB_GALOIS_CONTAINERS_HOST_LOCAL_ARRAY_HPP_

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <utility>

#include "pando-rt/export.h"
#include <pando-lib-galois/containers/host_indexed_map.hpp>
#include <pando-lib-galois/containers/host_local_storage.hpp>
#include <pando-lib-galois/loops/do_all.hpp>
#include <pando-lib-galois/utility/counted_iterator.hpp>
#include <pando-rt/containers/array.hpp>
#include <pando-rt/memory/address_translation.hpp>
#include <pando-rt/memory/allocate_memory.hpp>
#include <pando-rt/memory/global_ptr.hpp>

namespace galois {

/**
 * @brief a place and a memoryType for use in constructing a DistArray
 */
struct SizedMemoryLocation {
  pando::MemoryType memType;
  std::uint64_t size;
};

/**
 * @brief This is an array like container that has an array on each host */
template <typename T>
class HostLocalArray {
public:
  HostLocalArray() noexcept = default;

  HostLocalArray(const HostLocalArray&) = default;
  HostLocalArray(HostLocalArray&&) = default;

  ~HostLocalArray() = default;

  HostLocalArray& operator=(const HostLocalArray&) = default;
  HostLocalArray& operator=(HostLocalArray&&) = default;

  /**
   * @brief Takes in iterators with semantics like memoryType and a size to initialize the sizes of
   * the objects
   *
   * @tparam It the iterator type
   * @param[in] beg The beginning of the iterator to memoryType like objects
   * @param[in] end The end of the iterator to memoryType like objects
   * @param[in] size The size of the data to encapsulate in this abstraction
   */
  template <typename Range>
  [[nodiscard]] pando::Status initialize(Range range) {
    assert(range.size() == m_data.size());
    for (SizedMemoryLocation it = range.begin(); it != range.end(); it++) {
      it.
    }
    return pando::Status::Success;
  }

private:
  /// @brief The data structure storing the data
  galois::HostLocalStorage<galois::HostIndexedMap<pando::Array<T>>> m_data;
  /// @brief Stores the amount of data in the array, may be less than allocated
  uint64_t size_ = 0;
};
} // namespace galois

#endif // PANDO_LIB_GALOIS_CONTAINERS_HOST_LOCAL_ARRAY_HPP_
