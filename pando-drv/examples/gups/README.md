<!--
  ~ SPDX-License-Identifier: MIT
  ~ Copyright (c) 2023. University of Texas at Austin. All rights reserved.
  -->

```
SPDX-License-Identifier: MIT
Copyright (c) 2023 University of Washington
```

# Test Description

This test runs the RandomAccess microbenchmark.
The number of updates per thread (indirectly the total number of updates) should dictate how long the simulation runs.
This can be set with the THREAD_UPDATES variable to the makefile (default = 1024).

With the default parameters you should see the following output.

```
Simulation is complete, simulated time: 249.816 us
```
