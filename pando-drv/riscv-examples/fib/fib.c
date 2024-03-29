// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

int fib(int n)
{
    if (n <= 1)
        return n;
    return fib(n - 1) + fib(n - 2);
}
