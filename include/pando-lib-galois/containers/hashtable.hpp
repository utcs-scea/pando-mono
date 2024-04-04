// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_CONTAINERS_HASHTABLE_HPP_
#define PANDO_LIB_GALOIS_CONTAINERS_HASHTABLE_HPP_

#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <utility>

#include "pando-rt/export.h"
#include <pando-rt/containers/array.hpp>
#include <pando-rt/memory/address_translation.hpp>
#include <pando-rt/memory/allocate_memory.hpp>
#include <pando-rt/memory/global_ptr.hpp>
#include <pando-rt/utility/math.hpp>

namespace galois {

/**
 * @brief This is a HashTable implemented with quadratic probing.
 */
template <typename Key, typename T>
class HashTable {
public:
  struct Entry {
    Key key;
    T value;
    bool occupied = false;
  };

  // TODO(prydt) constructor with capacity, max load factor
  HashTable() noexcept = default;
  explicit HashTable(float maxLoad)
      : maxLoadFactor((maxLoad > 1.0)    ? 0.8
                      : (maxLoad < .001) ? 0.8
                                         : maxLoad) {}

  HashTable(const HashTable&) = default;
  HashTable(HashTable&&) = default;

  ~HashTable() = default;

  HashTable& operator=(const HashTable&) = default;
  HashTable& operator=(HashTable&&) = default;

  struct Iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Entry;
    using pointer = pando::GlobalPtr<Entry>;
    using reference = pando::GlobalRef<Entry>;

    using arr_iterator = typename pando::Array<Entry>::iterator;

    Iterator(arr_iterator iter, arr_iterator begin, arr_iterator end)
        : mIter(iter), mBegin(begin), mEnd(end) {}

    reference operator*() const {
      return *mIter;
    }

    pointer operator->() const {
      return &(*this);
    }

    Iterator& operator++() {
      mIter++;
      Entry e = *mIter;
      while (mIter != mEnd && !e.occupied) {
        mIter++;
        e = *mIter;
      }
      return *this;
    }

    Iterator operator++(int) {
      Iterator past = *this;
      ++(*this);
      return past;
    }

    Iterator& operator--() {
      mIter--;
      Entry e = *mIter;
      while (!e.occupied && mIter != mBegin) {
        mIter--;
        e = *mIter;
      }
      return *this;
    }

    Iterator operator--(int) {
      Iterator past = *this;
      --(*this);
      return past;
    }

    friend bool operator==(const Iterator& a, const Iterator& b) {
      return a.mIter == b.mIter;
    }

    friend bool operator!=(const Iterator& a, const Iterator& b) {
      return a.mIter != b.mIter;
    }

  private:
    arr_iterator mIter, mBegin, mEnd;
  };

  // TODO(prydt) add constant iterator

  // @brief initialize the capacity, place, and memory type of the hashtable.
  pando::Status initialize(std::size_t capacity, pando::Place place, pando::MemoryType memType) {
    if (capacity < 8 && capacity != 0) {
      capacity = 8;
    }
    const bool pow2 = (capacity & (capacity - 1)) == 0;
    const auto status = m_buffer.initialize(pow2 ? capacity : pando::up2(capacity), place, memType);
    if (status == pando::Status::Success) {
      m_buffer.fill(Entry{});
    }

    return status;
  }

  // @brief Initialize the hashtable with capacity in current location in main memory.
  pando::Status initialize(std::size_t capacity) {
    return initialize(capacity, pando::getCurrentPlace(), pando::MemoryType::Main);
  }

  void deinitialize() {
    m_buffer.deinitialize();
    m_size = 0;
  }

  // @brief Deinitialize the hashtable
  void deinitialize(pando::WaitGroup::HandleType wgh) {
    m_buffer.deinitialize(wgh);
    m_size = 0;
  }

  // @brief Resizes the backing array to `capacity`.
  pando::Status resize(std::size_t capacity) {
    if (capacity <= m_buffer.size()) {
      return pando::Status::Success;
    }

    const auto memType =
        m_buffer.data() == nullptr ? pando::MemoryType::Main : pando::memoryTypeOf(m_buffer.data());

    pando::Array<Entry> newBuffer;
    auto status = newBuffer.initialize(capacity, pando::localityOf(m_buffer.data()), memType);

    if (status != pando::Status::Success) {
      return pando::Status::BadAlloc;
    }

    newBuffer.fill(Entry{});

    for (std::size_t i = 0; i < m_buffer.size(); i++) {
      Entry e = m_buffer[i];
      if (e.occupied) {
        auto insert = bufferInsert(newBuffer, e.key, e.value);
        if (insert) {
          return pando::Status::Error;
        }
      }
    }

    std::swap(m_buffer, newBuffer);
    newBuffer.deinitialize();

    return pando::Status::Success;
  }

  // @brief If key is in hashtable, return true and place value into `value`
  // else return false
  bool get(const Key& key, T& value) {
    if (m_buffer.size() == 0) {
      return false;
    }
    Entry e = m_buffer[probe(m_buffer, key)];

    if (e.occupied && e.key == key) {
      value = e.value;
      return true;
    }

    return false;
  }

  /// @brief If key is in hashtable, return true else return false
  bool contains(const Key& key) {
    if (m_buffer.size() == 0) {
      return false;
    }
    Entry e = m_buffer[probe(m_buffer, key)];

    if (e.occupied && e.key == key) {
      return true;
    }
    return false;
  }

  // @brief Puts new `key` `value` pair into hashtable.
  pando::Status put(const Key& key, T value) {
    if (m_buffer.size() == 0 || loadFactor() >= maxLoadFactor) {
      auto status = resize(nextCapacity());
      if (status != pando::Status::Success) {
        return status;
      }
    }

    bool wasOccupied = bufferInsert(m_buffer, key, value);
    if (!wasOccupied) {
      m_size++;
    }
    return pando::Status::Success;
  }

  // @brief Clear all entries in hashtable.
  void clear() {
    m_size = 0;
    for (std::size_t i = 0; i < capacity(); i++) {
      Entry e = m_buffer[i];
      e.occupied = false;
      m_buffer[i] = e;
    }
  }

  // @brief Returns number of entries in the hashtable.
  std::size_t size() {
    return m_size;
  }

  // @brief Returns the current capacity of the hashtable.
  std::size_t capacity() {
    return m_buffer.size();
  }

  // @brief Returns the load factor of the hashtable.
  float loadFactor() {
    return static_cast<float>(m_size) / static_cast<float>(m_buffer.size());
  }

  Iterator begin() {
    auto i = m_buffer.begin();
    for (; i != m_buffer.end() && !static_cast<Entry>(*i).occupied; i++) {}
    return Iterator(i, m_buffer.begin(), m_buffer.end());
  }
  Iterator end() {
    return Iterator(m_buffer.end(), m_buffer.begin(), m_buffer.end());
  }

private:
  std::size_t m_size = 0;

  pando::Array<Entry> m_buffer;
  float maxLoadFactor = 0.8;

  /**
   * @warning note that i < cap
   * @warning note that the cap MUST be powers of two for this to work
   */
  constexpr std::uint64_t polynomial(std::uint64_t i, std::uint64_t cap) noexcept {
    // Since unsigned integer overflow is not undefined behavior this operation is safe.
    // By assumption adding 1 to i should not overflow
    std::uint64_t inner = (i % 2) ? ((i + 1) >> 1) * i : (i >> 1) * (i + 1);
    return inner % cap;
  }

  // returns the index a key would be in the hashtable
  // if it is present in the hashtable.
  std::size_t probe(pando::Array<Entry>& buf, const Key& key) {
    auto h = hashIndex(key, buf.size());
    Entry e = buf[h];
    std::size_t idx = h;

    // quadratic probing
    // This should not be able to loop due to the fact that the polynomial cycles in 2^n.
    for (std::size_t i = 1; e.occupied && e.key != key; i++) {
      idx = h + polynomial(i, buf.size());
      idx %= buf.size();

      e = buf[idx];
    }

    return idx;
  }

  bool bufferInsert(pando::Array<Entry>& buf, const Key& key, T value) {
    std::size_t idx = probe(buf, key);
    Entry e = buf[idx];

    // e is not occupied or e.key = key, due to probe
    bool wasOccupied = e.occupied;
    e.key = key;
    e.value = value;
    e.occupied = true;
    buf[idx] = e;
    return wasOccupied;
  }

  std::size_t nextCapacity() {
    if (m_buffer.size() == 0)
      return 8;
    return m_buffer.size() * 2;
  }

  inline std::size_t hashIndex(const Key& key, std::uint64_t size) {
    std::size_t i = std::hash<Key>{}(key);

    return i % size;
  }
};
} // namespace galois

#endif // PANDO_LIB_GALOIS_CONTAINERS_HASHTABLE_HPP_
