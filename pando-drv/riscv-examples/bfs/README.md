<!--
  ~ SPDX-License-Identifier: MIT
  ~ Copyright (c) 2023. University of Texas at Austin. All rights reserved.
  -->

<!--
  ~ SPDX-License-Identifier: MIT
  ~ Copyright (c) 2023. University of Washington. All rights reserved.
  -->

# Test Description

This is an implementation for a single 8-core pod
breadth-first-search using the direction optimized
algorithm [1].
It runs with a command processor (CP) responsible for
IO and moving input data onto PANDOHammer (PH) cores.

The CP runs a reference implementation to check against
the solution produced by the PH pod.

It can run arbitrary inputs, but by default it runs on a uniform random scale
12 graph with an average degree of 16.
The root node is 0 by default.

If this test fails, you will see ERROR messages showing
the mismatched outputs.
If this test passes, you will the following:

```txt
Opening graph file: /root/sst-drv-src/riscv-examples/bfs/sparse-inputs/u12k16.mtx
Reading file '/root/sst-drv-src/riscv-examples/bfs/sparse-inputs/u12k16.mtx'
Reading graph '/root/sst-drv-src/riscv-examples/bfs/sparse-inputs/u12k16.mtx': V = 4096, E = 130494
Converting to CSR
Vertices: 4096, Edges: 130494
Root vertex: 0
breadth first search iteration    0: traversed edges:        39, frontier size =         1
breadth first search iteration    1: traversed edges:      1262, frontier size =        39
breadth first search iteration    2: traversed edges:     34454, frontier size =      1050
breadth first search iteration    3: traversed edges:     94660, frontier size =      3003
breadth first search iteration    4: traversed edges:        79, frontier size =         3
breadth first search traversed 130494 edges
CP: waiting for PH threads to be ready: Cores: 8, Threads/Core: 16
PH: Core 2, Thread 7: g_V           = 4096
PH: Core 2, Thread 7: g_E           = 130494
PH: Core 2, Thread 7: g_fwd_offsets = 102203000
PH: Core 2, Thread 7: g_fwd_edges   = 102207010
PH: Core 2, Thread 7: g_rev_offsets = 102286710
PH: Core 2, Thread 7: g_rev_edges   = 10228a720
PH: Core 2, Thread 7: g_distance    = 102309e20
PH: Core 2, Thread 7: threads = 128, cores = 8, threads_per_core = 16
PH: Core 2, Thread 1: iteration 0: curr_frontier size = 1
PH: Core 2, Thread 1: curr_frontier is sparse
PH: Core 5, Thread 2: iteration 1: curr_frontier size = 39
PH: Core 5, Thread 2: curr_frontier is sparse
PH: Core 6, Thread 0: iteration 2: curr_frontier size = 1050
PH: Core 6, Thread 0: curr_frontier is sparse
PH: Core 6, Thread 2: iteration 3: curr_frontier size = 3003
PH: Core 6, Thread 2: curr_frontier is sparse
PH: Core 7, Thread 14: iteration 4: curr_frontier size = 3
PH: Core 7, Thread 14: curr_frontier is sparse
CP: all PH threads are done (128)
Simulation is complete, simulated time: 12.7318 ms
```

1. S. Beamer, K. Asanovic and D. Patterson,
"Direction-optimizing Breadth-First Search," SC '12:
Proceedings of the International Conference on High Performance Computing,
Networking, Storage and Analysis, Salt Lake City, UT,
USA, 2012, pp. 1-10, doi: 10.1109/SC.2012.50.
