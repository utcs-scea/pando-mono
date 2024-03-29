<!--
  ~ SPDX-License-Identifier: MIT
  ~ Copyright (c) 2023. University of Texas at Austin. All rights reserved.
  -->

SPDX-License-Identifier: MIT

First, download `Data01CSR.idmimg` into `inputs` directory from https://drive.google.com/file/d/1XjUGNuNvF3fwF8yRHPJbR13ASwfqfF-i/view?usp=sharing

To start simulation, execute `make run`.

When a thread finishes, it outputs some statistics like below
```
=========================== Compute thread   58 done ===========================
work: 96, sampled edges: 9562, sampled vertices: 2496
avg sampled edges: 99.60, avg sampled vertices: 26.00
V local accesses: 706, V remote accesses: 1790 (from IDM cache: 863 [48.21%])
E local accesses: 204 (from IDM cache: 86 [42.16%])
E remote accesses: 3329 (from IDM cache: 1630 [48.96%])
Root Local, Root Remote, 7, 409
================================================================================
```

## Configurations
- `SIM_PXN`. The simulation is performed as if there are `SIM_PXN` PXNs. Default: 8.
- `CACHE_SIZE`. The number of entries in the caches. Default: 512.
- `PREFETCH_AHEAD_MIN`/`PREFETCH_AHEAD_MAX`. The look ahead range to do prefetch. Default: 2/4.
