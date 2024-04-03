#!/bin/bash
# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.

HOST_THREADS="${HOST_THREADS:-16}"
HOSTS="${HOSTS:-1}"
PROCS="${PROCS:-8}"
CORES="${CORES:-4}"
HARTS="${HARTS:-16}"
THREADD=$((${HOST_THREADS} * ${HOSTS} / ${PROCS}))
THREADS="${THREADS:-${THREADD}}"
STACK_SIZE="${STACK_SIZE:-8192}"
# 33MB Pod memory size by default
POD_MEMORY_SIZE="${POD_MEMORY_SIZE:-33554432}"
# 96GB Main memory size by default
MAIN_MEMORY_SIZE="${MAIN_MEMORY_SIZE-103079215104}"
LAUNCH_SCRIPT="${LAUNCH_SCRIPT:-gasnetrun_mpi}"
EXE="${EXE:-dockerbuild/workflows/influence_maximization/src/wf4}"

K="${K:-5}"
RRR_SETS="${RRR_SETS:-1000}"
SEED="${SEED:-9801}"
COMMERCIAL_FILE="${COMMERCIAL_FILE:-graphs/influence_maximization/scale-2/commercial.csv}"
CYBER_FILE="${CYBER_FILE:-graphs/influence_maximization/scale-2/cyber.csv}"
SOCIAL_FILE="${SOCIAL_FILE:-graphs/influence_maximization/scale-2/social.csv}"
USES_FILE="${USES_FILE:-graphs/influence_maximization/scale-2/uses.csv}"
LOGFILE="${LOGFILE:-data/wf4_scale-2_run.txt}"

DISABLE_K2="${DISABLE_K2:-1}"
DISABLE_K3="${DISABLE_K3:-1}"

export PANDO_PREP_NUM_CORES=$CORES
export PANDO_PREP_NUM_HARTS=$HARTS
export PANDO_PREP_L1SP_HART=$STACK_SIZE
export PANDO_PREP_L2SP_POD=$POD_MEMORY_SIZE
export PANDO_PREP_MAIN_NODE=$MAIN_MEMORY_SIZE

export MPIRUN_CMD="mpirun -np %N -host localhost:${HOST_THREADS} --map-by slot:PE=${THREADS} %P %A"
# TODO(Patrick) add in `-N ${HOSTS}`
${LAUNCH_SCRIPT} \
    -n ${PROCS} \
    -v \
    -E PANDO_PREP_NUM_CORES,PANDO_PREP_NUM_HARTS,PANDO_PREP_L1SP_HART,PANDO_PREP_L2SP,PANDO_PREP_MAINMEMORY \
    "${EXE}" \
    -k ${K} \
    -r ${RRR_SETS} \
    -s ${SEED} \
    -c "${COMMERCIAL_FILE}" \
    -y "${CYBER_FILE}" \
    -o "${SOCIAL_FILE}" \
    -u "${USES_FILE}" \
    -2 "${DISABLE_K2}" \
    -3 "${DISABLE_K3}" \
    -- \
    2>&1 | tee "${LOGFILE}"
