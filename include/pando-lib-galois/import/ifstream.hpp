// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_IMPORT_IFSTREAM_HPP_
#define PANDO_LIB_GALOIS_IMPORT_IFSTREAM_HPP_

#include <pando-rt/export.h>

#include <cstdint>

#include <pando-rt/pando-rt.hpp>

namespace galois {

/**
 * @brief A class that uses the correct DRV api in order to read files.
 */
class ifstream {
  /// @brief stores a file descriptor
  int m_fd = -1;
  /// @brief stores the current location the stream is at
  std::uint64_t m_pos = 0;
  /// @brief stores the last error to be returned during implicit depromotion
  pando::Status m_err = pando::Status::Success;

public:
  ifstream() noexcept : m_fd(-1), m_pos(0), m_err(pando::Status::Success) {}

  ifstream(const ifstream&) = default;
  ifstream(ifstream&&) = default;

  ~ifstream() = default;

  ifstream& operator=(const ifstream&) = default;
  ifstream& operator=(ifstream&&) = default;

  [[nodiscard]] pando::Status open(const char* filepath);

  /**
   * @brief closes the underlying file
   */
  void close();

  /**
   * @brief Returns the current status
   */
  constexpr pando::Status status() noexcept {
    return m_err;
  }

  /**
   * @brief implicitly converts the current status to a boolean comparing it to
   * pando::Status::Success
   */
  constexpr operator bool() noexcept {
    return m_err == pando::Status::Success;
  }

  /**
   * @brief gets the current character and increments the current position
   * @warning if the file read goes over the m_pos does not change
   */
  ifstream& get(char& c);

  /**
   * @brief decrements the current position if it is valid.
   */
  ifstream& unget();

  /**
   * @brief Reads the up to n characters and stops if the file ends.
   *
   * @param[in] buf The buffer to place values
   * @param[in] n   The maximum number of characters that could be read
   * @warning n should be less than or equal to the buffer size
   */
  ifstream& read(char* buf, std::uint64_t n);

  /**
   * @brief Gets the current line.
   *
   * @param[in] buf   The buffer to place values
   * @param[in] n     The maximum number of characters that could be read
   * @param[in] delim The deliminator that serves as a newline
   */
  ifstream& getline(char* buf, std::uint64_t n, char delim);

  /**
   * @brief reads a uint64_t
   * @param[out] val a reference to the output
   */
  ifstream& operator>>(std::uint64_t& val);
};
static_assert(std::is_trivially_copyable<ifstream>::value);
} // namespace galois

#endif // PANDO_LIB_GALOIS_IMPORT_IFSTREAM_HPP_
