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

## Sample Command

```bash
# On PREP:
PANDO_PREP_MAIN_NODE=17179869184 PANDO_PREP_NUM_CORES=2 \
gasnetrun_mpi -n 2 ./dockerbuild/microbench/triangle-counting/src/tc -i \
graphs/rmat_571919_seed1_scale5_nV32_nE153.el -v 32
```
