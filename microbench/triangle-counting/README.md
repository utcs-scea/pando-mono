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

PANDO_RT_ENABLE_MEM_TRACE=INTER-PXN PANDO_RT_ENABLE_MEM_STAT=ON \
BUILD_MICROBENCH=ON make setup
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

## Benchmark: Task Decomposition

- `-i`: Required: path to graph file
- `-v`: Required: num vertices in graph
- `-c`: Optional (Default: 0): Specify how chunk tasks:
  - `-c 0`: NO chunking -- This is always chosen if `-l = False`
  - `-c 1`: Chunk Vertices
  - `-c 2`: Chunk Edges
- `-g`: Optional (Default: 0): Specify graph type:
  - `-g 0`: DistLocalCSR
  - `-g 1`: MirroredDistLocalCSR
  - `-g 2`: DistArrayCSR -- Chunking automatically off

```bash
# On PREP: Runs TC (no chunking) on DistArrayCSR
MPIRUN_CMD="mpirun -np %N -host localhost:32 --mca btl self,vader,tcp \
--map-by slot:PE=16 \
--use-hwthread-cpus %P %A" PANDO_PREP_MAIN_NODE=17179869184 \
./scripts/preprun_mpi.sh -c 32 \
-n 2 ./dockerbuild/microbench/triangle-counting/src/tc -i \
graphs/rmat_571919_seed1_scale5_nV32_nE153.el -v 32 -c 0 -g 0
```
