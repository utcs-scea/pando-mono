```
SPDX-License-Identifier: MIT
Copyright (c) 2023 University of Washington
```

# Test Description

This tests functions for hardware threads (harts) to query their place in the universe (PANDO system).
The following functions are tested:

- `myPXNId()`
- `myPodId()`
- `myCoreId()`
- `myThreadId()`
- `numPXNs()`
- `numPXNPods()`
- `numPodCores()`
- `myCoreThreads()`

By default you should the following output:

```
my pxn:  0/ 1, my pod:  0/ 1, my core:  0/ 8, my thread:  0/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  1/ 8, my thread:  0/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  2/ 8, my thread:  0/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  3/ 8, my thread:  0/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  4/ 8, my thread:  0/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  5/ 8, my thread:  0/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  6/ 8, my thread:  0/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  7/ 8, my thread:  0/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  0/ 8, my thread:  1/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  1/ 8, my thread:  1/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  2/ 8, my thread:  1/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  3/ 8, my thread:  1/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  4/ 8, my thread:  1/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  5/ 8, my thread:  1/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  6/ 8, my thread:  1/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  7/ 8, my thread:  1/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  0/ 8, my thread:  2/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  1/ 8, my thread:  2/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  2/ 8, my thread:  2/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  3/ 8, my thread:  2/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  4/ 8, my thread:  2/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  5/ 8, my thread:  2/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  6/ 8, my thread:  2/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  7/ 8, my thread:  2/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  0/ 8, my thread:  3/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  1/ 8, my thread:  3/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  2/ 8, my thread:  3/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  3/ 8, my thread:  3/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  4/ 8, my thread:  3/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  5/ 8, my thread:  3/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  6/ 8, my thread:  3/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  7/ 8, my thread:  3/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  0/ 8, my thread:  4/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  1/ 8, my thread:  4/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  2/ 8, my thread:  4/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  3/ 8, my thread:  4/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  4/ 8, my thread:  4/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  5/ 8, my thread:  4/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  6/ 8, my thread:  4/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  7/ 8, my thread:  4/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  0/ 8, my thread:  5/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  1/ 8, my thread:  5/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  2/ 8, my thread:  5/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  3/ 8, my thread:  5/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  4/ 8, my thread:  5/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  5/ 8, my thread:  5/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  6/ 8, my thread:  5/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  7/ 8, my thread:  5/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  0/ 8, my thread:  6/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  1/ 8, my thread:  6/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  2/ 8, my thread:  6/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  3/ 8, my thread:  6/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  4/ 8, my thread:  6/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  5/ 8, my thread:  6/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  6/ 8, my thread:  6/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  7/ 8, my thread:  6/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  0/ 8, my thread:  7/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  1/ 8, my thread:  7/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  2/ 8, my thread:  7/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  3/ 8, my thread:  7/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  4/ 8, my thread:  7/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  5/ 8, my thread:  7/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  6/ 8, my thread:  7/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  7/ 8, my thread:  7/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  0/ 8, my thread:  8/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  1/ 8, my thread:  8/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  2/ 8, my thread:  8/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  3/ 8, my thread:  8/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  4/ 8, my thread:  8/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  5/ 8, my thread:  8/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  6/ 8, my thread:  8/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  7/ 8, my thread:  8/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  0/ 8, my thread:  9/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  1/ 8, my thread:  9/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  2/ 8, my thread:  9/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  3/ 8, my thread:  9/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  4/ 8, my thread:  9/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  5/ 8, my thread:  9/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  6/ 8, my thread:  9/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  7/ 8, my thread:  9/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  0/ 8, my thread: 10/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  1/ 8, my thread: 10/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  2/ 8, my thread: 10/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  3/ 8, my thread: 10/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  4/ 8, my thread: 10/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  5/ 8, my thread: 10/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  6/ 8, my thread: 10/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  7/ 8, my thread: 10/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  0/ 8, my thread: 11/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  1/ 8, my thread: 11/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  2/ 8, my thread: 11/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  3/ 8, my thread: 11/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  4/ 8, my thread: 11/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  5/ 8, my thread: 11/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  6/ 8, my thread: 11/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  7/ 8, my thread: 11/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  0/ 8, my thread: 12/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  1/ 8, my thread: 12/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  2/ 8, my thread: 12/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  3/ 8, my thread: 12/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  4/ 8, my thread: 12/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  5/ 8, my thread: 12/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  6/ 8, my thread: 12/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  7/ 8, my thread: 12/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  0/ 8, my thread: 13/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  1/ 8, my thread: 13/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  2/ 8, my thread: 13/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  3/ 8, my thread: 13/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  4/ 8, my thread: 13/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  5/ 8, my thread: 13/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  6/ 8, my thread: 13/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  7/ 8, my thread: 13/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  0/ 8, my thread: 14/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  1/ 8, my thread: 14/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  2/ 8, my thread: 14/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  3/ 8, my thread: 14/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  4/ 8, my thread: 14/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  5/ 8, my thread: 14/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  6/ 8, my thread: 14/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  7/ 8, my thread: 14/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  0/ 8, my thread: 15/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  1/ 8, my thread: 15/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  2/ 8, my thread: 15/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  3/ 8, my thread: 15/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  4/ 8, my thread: 15/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  5/ 8, my thread: 15/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  6/ 8, my thread: 15/16
my pxn:  0/ 1, my pod:  0/ 1, my core:  7/ 8, my thread: 15/16
Simulation is complete, simulated time: 128 ns
```
