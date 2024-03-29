// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include <atomic>
extern "C" int snprintf_input()
{
    static std::atomic<int> x = 0;
    return x.fetch_add(1, std::memory_order_relaxed);
}
