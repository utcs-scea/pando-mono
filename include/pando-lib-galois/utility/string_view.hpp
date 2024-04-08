// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_UTILITY_STRING_VIEW_HPP_
#define PANDO_LIB_GALOIS_UTILITY_STRING_VIEW_HPP_

#include <pando-rt/export.h>

#include <cstdint>
#include <cstdlib>

#include <pando-rt/containers/array.hpp>
#include <pando-rt/pando-rt.hpp>

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

  /**
   * @brief constructor that creates memory from x86 heap because of how parsing strings works
   * @warning When using this memory must be tracked by the programmer and freed as appropriate
   */
  explicit StringView(pando::Array<char> str) {
    size_ = 0;
    for (size_ = 0; size_ < str.size() && str[size_] != '\0'; size_++) {}
    char* placeholder = static_cast<char*>(malloc(sizeof(char) * (size_ + 1)));
    for (size_t i = 0; i < size_; i++) {
      placeholder[i] = str[i];
    }
    placeholder[size_] = '\0';
    start_ = static_cast<const char*>(placeholder);
  }

  pando::Array<char> toArray() {
    pando::Status err;
    pando::Array<char> str{};
    err = str.initialize(size_);
    if (err != pando::Status::Success) {
      return str;
    }
    for (std::uint64_t i = 0; i < static_cast<std::uint64_t>(size_); i++) {
      str[i] = start_[i];
    }
    return str;
  }

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

  /**
   * @brief Parse US date from StringView
   */
  time_t getUSDate() {
    auto charToInt = [*this](size_t& pos) {
      int result = 0;
      char delimiter = '/';
      for (size_t i = pos; i < size_ && start_[i] != delimiter; ++i) {
        result = result * 10 + (start_[pos++] - '0');
      }
      return result;
    };
    std::tm timeStruct = {};
    size_t pos = 0;

    timeStruct.tm_mon = charToInt(pos);
    pos++;

    timeStruct.tm_mday = charToInt(pos);
    pos++;

    timeStruct.tm_year = charToInt(pos);

    timeStruct.tm_year -= 1900;
    timeStruct.tm_mon--;

    timeStruct.tm_hour = 0;
    timeStruct.tm_min = 0;
    timeStruct.tm_sec = 0;

    return std::mktime(&timeStruct);
  }

  /**
   * @brief Parse Double from StringView
   */
  double getDouble() {
    size_t i = 0;
    double result = 0.0;
    bool negative = false;

    if (start_[i] == '-') {
      negative = true;
      i++;
    }

    while (i < size_ && start_[i] != '.') {
      result = result * 10.0 + (start_[i] - '0');
      i++;
    }

    if (start_[i] == '.') {
      i++;
      double fraction = 0.0;
      double divisor = 10.0;

      while (i < size_) {
        fraction += (start_[i] - '0') / divisor;
        divisor *= 10.0;
        i++;
      }

      result += fraction;
    }

    if (negative) {
      result = -result;
    }

    return result;
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
