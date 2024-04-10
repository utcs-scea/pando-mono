<!--
  ~ SPDX-License-Identifier: MIT
  ~ Copyright (c) 2023. University of Texas at Austin. All rights reserved.
  -->

# Triangle Counting

> Counts number of unique triangles in a graph

## Setup Instructions

```bash
git submodule update --init --recursive
make dependencies
make hooks
make docker-image
make docker
# These commands are run in the container `make docker` drops you into
# If necessary, run: setarch -R /bin/bash

make setup
make -C dockerbuild -j8
make run-tests
```

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

- `-i`: Required: path to graph file
- `-v`: Required: num vertices in graph
- `-l`: Optional (Default: False): Use DistLocalCSR ... defaults to DistArrayCSR
- `-c`: Optional (Default: 0): In DistLocalCSR, specify how chunk tasks:
  - `-c 0`: NO chunking -- This is always chosen if `-l = False`
  - `-c 1`: Chunk Edges
  - `-c 2`: Chunk Vertices

```bash
# On PREP: Runs TC (no chunking) on DistArrayCSR
PANDO_PREP_MAIN_NODE=17179869184 PANDO_PREP_NUM_CORES=2 \
gasnetrun_mpi -n 2 ./dockerbuild/microbench/triangle-counting/src/tc -i \
graphs/rmat_571919_seed1_scale5_nV32_nE153.el -v 32
```
