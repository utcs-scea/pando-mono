<!--
  ~ SPDX-License-Identifier: MIT
  ~ Copyright (c) 2023. University of Texas at Austin. All rights reserved.
  -->

```
SPDX-License-Identifier: MIT
Copyright (c) 2023 University of Washington
```

# Test Description

This is an implementation for a single 8-core pod breadth-first-search using the direction optimized algorithm [1].
Core 0/Thread 0 runs a reference implementation is run natively to check against the solution produced by all cores.

It can run arbitrary inputs, but by default it runs on a uniform random scale 12 graph with an average degree of 16.
The root node is 0 by default.

If this test fails, you will see ERROR messages showing the mismatched outputs.
If this test passes, you will the following:
```
Reading file '/mnt/users/ssd3/homes/mrutt/work/AGILE/drv/examples/bfs/sparse-inputs/u12k16.mtx'
Reading graph '/mnt/users/ssd3/homes/mrutt/work/AGILE/drv/examples/bfs/sparse-inputs/u12k16.mtx': V = 4096, E = 130494
Converting to CSR
breadth first search iteration    0: traversed edges:        39, frontier size =         1
breadth first search iteration    1: traversed edges:      1262, frontier size =        39
breadth first search iteration    2: traversed edges:     34454, frontier size =      1050
breadth first search iteration    3: traversed edges:     94660, frontier size =      3003
breadth first search iteration    4: traversed edges:        79, frontier size =         3
breadth first search traversed 130494 edges
Finished reading graph
Starting BFS (Push-Pull)
Iteration  0:   1 in frontier
Iteration  0:  39 mf, 130455 mu, fwd
Iteration  1:  39 in frontier
Iteration  1: 1262 mf, 129193 mu, fwd
Iteration  2: 1050 in frontier
Iteration  2: 34454 mf, 94739 mu, rev
Iteration  3: 3003 in frontier
Iteration  3: curr_frontier.size()=3003, (V/20) = (4096/20) = 204, rev
Iteration  4:   3 in frontier
Iteration  4: curr_frontier.size()=  3, (V/20) = (4096/20) = 204, fwd
Simulation is complete, simulated time: 1.06719 ms
```

1. S. Beamer, K. Asanovic and D. Patterson, "Direction-optimizing Breadth-First Search," SC '12: Proceedings of the International Conference on High Performance Computing, Networking, Storage and Analysis, Salt Lake City, UT, USA, 2012, pp. 1-10, doi: 10.1109/SC.2012.50.
