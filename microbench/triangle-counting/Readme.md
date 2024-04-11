<!--
  ~ SPDX-License-Identifier: MIT
  ~ Copyright (c) 2023. University of Texas at Austin. All rights reserved.
  -->

# Triangle-Counting (TC)

## Microbenchmark: BenchRemoteAccesses

- `-y`: Default `False`
  - Use -y for move-data-to-compute (via remote references).
  - Default: move-compute-to-data (via `executeOn`)
- **Sample Command in DRVX** with:

  ```bash
  ./scripts/run-drv.sh -n 2 \
  ./dockerbuild-drv/microbench/triangle-counting/src/libbench_remote_access.so -y
  ```

## Microbenchmark: DistArrayCSR vs DistLocalCSR

- TBD

## Benchmark: Task Decomposition

- TBD
