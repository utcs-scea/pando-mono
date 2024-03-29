<!--
  ~ SPDX-License-Identifier: MIT
  ~ Copyright (c) 2023. University of Texas at Austin. All rights reserved.
  -->

```
SPDX-License-Identifier: MIT
Copyright (c) 2023 University of Washington
Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
```

# Test Description

This test runs the RandomAccess microbenchmark with multiple pxns and pods.
The number of updates per thread (indirectly the total number of updates) should dictate how long the simulation runs.
This can be set with the THREAD_UPDATES variable to the makefile (default = 1024).

With num_pxn=2, pxn_pods=2 and with other default parameters (for instance pod-cores=8) you should see the following output.

```
Simulation is complete, simulated time: 1.04864 ms
```
