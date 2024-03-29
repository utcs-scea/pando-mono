// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include "breadth_first_search_graph.hpp"
#include <set>
#include <stdio.h>
void breadth_first_search_graph(int root, int V, int E, const std::vector<int>& fwd_offsets,
                                const std::vector<int>& fwd_nonzeros, std::vector<int>& distance) {
  int traversed = 0;
  distance = std::vector<int>(V, -1);
  distance[root] = 0;
  std::set<int> curr_frontier, next_frontier;
  curr_frontier.insert(root);
  int d = 0;
  while (!curr_frontier.empty()) {
    d++;
    int traversed_this_iter = 0;
    for (int src : curr_frontier) {
      int start = fwd_offsets[src];
      int stop = fwd_offsets[src + 1];
      traversed_this_iter += stop - start;
      for (int nz = start; nz < stop; nz++) {
        int dst = fwd_nonzeros[nz];
        if (distance[dst] == -1) {
          distance[dst] = d;
          next_frontier.insert(dst);
        }
      }
    }

    printf("breadth first search iteration %4d: traversed edges: %9d, frontier size = %9zu\n",
           d - 1, traversed_this_iter, curr_frontier.size());

    curr_frontier = std::move(next_frontier);
    next_frontier.clear();

    traversed += traversed_this_iter;
  }
  printf("breadth first search traversed %d edges\n", traversed);
}
