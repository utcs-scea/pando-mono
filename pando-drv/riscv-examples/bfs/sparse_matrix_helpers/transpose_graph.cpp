// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include "transpose_graph.hpp"
#include <algorithm>
int transpose_graph(int V, int E, const std::vector<int>& fwd_offsets,
                    const std::vector<int>& fwd_nonzeros,
                    /* outputs */
                    std::vector<int>& rev_offsets, std::vector<int>& rev_nonzeros) {
  rev_offsets = std::vector<int>(V + 1);
  rev_nonzeros = std::vector<int>(E);
  std::vector<std::vector<int>> columns(V + 1);
  for (int s = 0; s < V; s++) {
    int start = fwd_offsets[s];
    int stop = fwd_offsets[s + 1];
    for (int k = start; k < stop; k++) {
      int d = fwd_nonzeros[k];
      columns[d].push_back(s);
    }
  }
  int sum = 0;
  for (size_t d = 0; d < rev_offsets.size(); d++) {
    rev_offsets[d] = sum;
    sum += columns[d].size();
  }
  int n = 0;
  for (int d = 0; d < V; d++) {
    std::sort(columns[d].begin(), columns[d].end());
    for (int s : columns[d]) {
      rev_nonzeros[n++] = s;
    }
  }
  return 0;
}
