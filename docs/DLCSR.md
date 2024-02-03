<!--
  ~ SPDX-License-Identifier: MIT
  ~ Copyright (c) 2023. University of Texas at Austin. All rights reserved.
  -->

# Distributed Local CSR

Creates a per PXN CSR graph

The goal is to enable efficient computation by
using a graph where some of the edges are
guaranteed to be local to that PXN.
Currently the only supported partitioning
policy is outgoing edge cut, meaning all
edges of each vertex are on that local PXN.
The theory is by creating such partitions
the local computations can be handled
quickly and remote computation can be batched.

## Constructing DLCSR

We have only tested using 16GB for larger data file sizes.
Please make sure to use
`PANDO_PREP_MAIN_NODE=17179869184` in your environment.
We are investigating how to curtail this.
