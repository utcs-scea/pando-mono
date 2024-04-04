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
EXE="${EXE:-dockerbuild-drv/workflows/influence_maximization/src/libwf4.so}"

K="${K:-10}"
RRR_SETS="${RRR_SETS:-1000}"
SEED="${SEED:-9801}"
COMMERCIAL_FILE="${COMMERCIAL_FILE:-graphs/influence_maximization/scale-2/commercial.csv}"
CYBER_FILE="${CYBER_FILE:-graphs/influence_maximization/scale-2/cyber.csv}"
SOCIAL_FILE="${SOCIAL_FILE:-graphs/influence_maximization/scale-2/social.csv}"
USES_FILE="${USES_FILE:-graphs/influence_maximization/scale-2/uses.csv}"
LOGFILE="${LOGFILE:-data/wf4_drv_scale-2.txt}"

DISABLE_K2="${DISABLE_K2:-1}"
DISABLE_K3="${DISABLE_K3:-1}"

sst -n ${HOST_THREADS} \
    "${LAUNCH_SCRIPT}" -- \
    --with-command-processor="${EXE}" \
    --num-pxn=${PROCS} \
    --pod-cores=${CORES} \
    --core-threads=${HARTS} \
    --drvx-stack-in-l1sp \
    --pxn-dram-size=${MAIN_MEMORY_SIZE} \
    "${EXE}" \
    -k ${K} \
    -r ${RRR_SETS} \
    -s ${SEED} \
    -c "${COMMERCIAL_FILE}" \
    -y "${CYBER_FILE}" \
    -o "${SOCIAL_FILE}" \
    -u "${USES_FILE}" \
    -2 "${DISABLE_K2}" \
    -3 "${DISABLE_K3}"
