// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <unistd.h>

#include <fcntl.h>
#include <limits.h>
#include <pando-lib-galois/import/ifstream.hpp>

namespace galois {
pando::Status galois::ifstream::open(const char* filepath) {
  if (m_fd != -1) {
    return pando::Status::AlreadyInit;
  }

  m_fd = ::open(filepath, O_RDONLY);

  if (m_fd < 0) {
    return pando::Status::InvalidValue;
  }

  return pando::Status::Success;
}

void ifstream::close() {
  if (m_fd == -1) {
    m_err = pando::Status::NotInit;
    return;
  }
  ::close(m_fd);
  m_fd = -1;
  m_pos = 0;
  m_err = pando::Status::Success;
}

ifstream& ifstream::get(char& c) {
  if (m_fd == -1) {
    m_err = pando::Status::NotInit;
    return *this;
  }
  ssize_t nRd = pread(m_fd, &c, 1, m_pos);
  if (nRd == 0) {
    m_err = pando::Status::OutOfBounds;
    return *this;
  }
  m_pos++;
  return *this;
}

ifstream& ifstream::unget() {
  if (m_pos == 0) {
    m_err = pando::Status::OutOfBounds;
    return *this;
  }
  m_pos--;
  return *this;
}

ifstream& ifstream::read(char* str, std::uint64_t n) {
  if (m_fd == -1) {
    m_err = pando::Status::NotInit;
    return *this;
  }
  ssize_t err;
  do {
    err = pread(m_fd, str, std::min(n, static_cast<std::uint64_t>(SSIZE_MAX)), m_pos);
    if (err == -1) {
      m_err = pando::Status::Error;
    } else if (err == 0) {
      m_err = pando::Status::OutOfBounds;
    } else {
      m_err = pando::Status::Success;
      m_pos += err;
      n -= err;
      str += err;
    }
  } while (err != -1 && err != 0 && n > 0);
  return *this;
}

ifstream& ifstream::getline(char* str, std::uint64_t n, char delim) {
  std::uint64_t i = 0;
  while (i < (n - 1) && this->get(str[i]) && str[i] != delim) {
    i++;
  }
  str[i] = '\0';
  return *this;
}

ifstream& ifstream::operator>>(std::uint64_t& val) {
  val = 0;
  char c;
  while (this->get(c) && isspace(c)) {}
  do {
    if (isdigit(c)) {
      val *= 10;
      val += (c - '0');
    } else if (c == '_') {
      continue;
    } else {
      this->unget();
      return *this;
    }
  } while (this->get(c));
  return *this;
}

} // namespace galois
