#!/bin/bash

# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT

DRV_TESTS=$HOME/pando/sst/pando-drv/tests
numactl --physcpubind=0-95 --membind=0 sst -n 24 --verbose --model-options=" $HOME/pando/sst/pando-drv/examples/stream/stream.so" $DRV_TESTS/drv-multicore-bus-ramu-test.py
#mpirun -n 8 sst -n 24 --verbose --model-options=" $HOME/pando/sst/pando-drv/examples/stream/stream.so" $DRV_TESTS/drv-multicore-bus-ramu-test.py
