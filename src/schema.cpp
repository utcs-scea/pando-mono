// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <pando-lib-galois/import/schema.hpp>

#include <pando-rt/pando-rt.hpp>

pando::Vector<galois::StringView> galois::splitLine(const char* line, char delim,
                                                    uint64_t numTokens) {
  uint64_t ndx = 0, start = 0, end = 0;
  pando::Vector<galois::StringView> tokens;
  PANDO_CHECK(tokens.initialize(numTokens));

  for (; line[end] != '\0' && line[end] != '\n'; end++) {
    if (line[end] == delim) {
      tokens[ndx] = galois::StringView(line + start, end - start);
      start = end + 1;
      ndx++;
    }
  }
  tokens[numTokens - 1] = galois::StringView(line + start, end - start); // flush last token
  return tokens;
}
