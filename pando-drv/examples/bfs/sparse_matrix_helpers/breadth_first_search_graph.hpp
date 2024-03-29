// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#pragma once
#include <vector>
void breadth_first_search_graph(int root, int V, int E, const std::vector<int>& fwd_offsets,
                                const std::vector<int>& fwd_nonzeros, std::vector<int>& distance);
