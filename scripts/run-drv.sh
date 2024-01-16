#!/bin/bash
# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.

HOST_THREADS="${HOST_THREADS:-16}"
HOSTS="${HOSTS:-1}"
PROCS="${PROCS:-2}"
CORES="${CORES:-8}"
HARTS="${HARTS:-16}"
THREADD=$((${HOST_THREADS} * ${HOSTS} / ${PROCS}))
THREADS="${THREADS:-${THREADD}}"
# 8GB Main memory size by default
MAIN_MEMORY_SIZE="${MAIN_MEMORY_SIZE-8589934592}"
LAUNCH_SCRIPT="${LAUNCH_SCRIPT:-pando-drv/tests/PANDOHammerDrvX.py}"
EXE="${EXE:-dockerbuild-drv/pando-rt/examples/libhelloworld.so}"

sst -n ${HOST_THREADS} \
	"${LAUNCH_SCRIPT}" -- \
	--with-command-processor="${EXE}" \
	--num-pxn=${PROCS} \
	--pod-cores=${CORES} \
	--core-threads=${HARTS} \
	--drvx-stack-in-l1sp \
	--pxn-dram-size=${MAIN_MEMORY_SIZE} \
	"${EXE}"
