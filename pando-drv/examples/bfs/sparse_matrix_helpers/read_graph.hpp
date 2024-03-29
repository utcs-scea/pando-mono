// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#pragma once
#include <string>
#include <vector>
#include <utility>
int read_graph(
    const std::string &graph,
    /* outputs */
    int *V,
    int *E,
    std::vector<int> &offsets,
    std::vector<int> &nonzeros);

int read_sparse_matrix(
    const std::string &graph,
    /* outputs */
    int *V,
    int *E,
    std::vector<int> &offsets,
    std::vector<std::pair<int, float>> &nonzeros);
