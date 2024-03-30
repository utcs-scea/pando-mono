#!/bin/bash
# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.

HOST_THREADS="${HOST_THREADS:-16}"
HOSTS="${HOSTS:-1}"
PROCS="${PROCS:-2}"
CORES="${CORES:-4}"
HARTS="${HARTS:-16}"
THREADD=$((${HOST_THREADS} * ${HOSTS} / ${PROCS}))
THREADS="${THREADS:-${THREADD}}"
STACK_SIZE="${STACK_SIZE:-32768}"
LAUNCH_SCRIPT="${LAUNCH_SCRIPT:-gasnetrun_mpi}"
EXE="${EXE:-dockerbuild/pando-rt/examples/helloworld}"
EXE_ARGS="${EXE_ARGS:-}"
LOGFILE="${LOGFILE:-data/run.txt}"

export PANDO_PREP_NUM_CORES=$CORES
export PANDO_PREP_NUM_HARTS=$HARTS
export PANDO_PREP_L1SP_HART=$STACK_SIZE

export MPIRUN_CMD="mpirun -np %N -host localhost:${HOST_THREADS} --map-by slot:PE=${THREADS} %P %A"
# TODO(Patrick) add in `-N ${HOSTS}`
${LAUNCH_SCRIPT} \
    -n ${PROCS} \
    -E PANDO_PREP_NUM_CORES,PANDO_PREP_NUM_HARTS,PANDO_PREP_L1SP_HART \
    "${EXE}" \
    ${EXE_ARGS} \
    -- \
    2>&1 | tee "${LOGFILE}"
