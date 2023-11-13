// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_UTILITY_STRING_VIEW_HPP_
#define PANDO_LIB_GALOIS_UTILITY_STRING_VIEW_HPP_

#include <cstdint>
#include <cstdlib>

namespace galois {

class StringView {
private:
  const char* start_;
  size_t size_;

public:
  StringView() : start_(nullptr), size_(0) {}
  explicit StringView(const char* start) : start_(start) {
    size_ = 0;
    for (const char* c = start_; *c != '\0'; c++) {
      size_++;
    }
  }
  StringView(const char* start, size_t size) : start_(start), size_(size) {}

  friend bool operator==(const StringView& a, const StringView& b) {
    bool equal = a.size_ == b.size_;
    if (!equal) {
      return equal;
    }
    for (size_t i = 0; i < a.size_; i++) {
      if (a.start_[i] != b.start_[i]) {
        return false;
      }
    }
    return true;
  }

  uint64_t getU64() {
    uint64_t res = 0;
    for (size_t i = 0; i < size_; i++) {
      res = 10 * res + (start_[i] - '0');
    }
    return res;
  }

  size_t size() {
    return size_;
  }

  bool empty() {
    return size() == 0;
  }

  const char* get() {
    return start_;
  }
};

} // namespace galois

#endif // PANDO_LIB_GALOIS_UTILITY_STRING_VIEW_HPP_
