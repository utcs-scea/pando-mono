#!/bin/bash

module purge
module load gcc/12.2.0
module load cmake/3.24.2

export PANDO_TACC_DEPENDENCY_DIR=$WORK/mono-install
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PANDO_TACC_DEPENDENCY_DIR
export GASNet_ROOT=$PANDO_TACC_DEPENDENCY_DIR
export QTHREADS_ROOT=$PANDO_TACC_DEPENDENCY_DIR
export SRC_DIR=.
export BUILD_DIR=build
export BUILD_MICROBENCH=ON
