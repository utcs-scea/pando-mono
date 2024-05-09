#!/bin/bash
# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.

set -o errexit
set -o nounset
set -o pipefail

#
# Help function
#
function show_help() {
    echo "
usage: preprun -n <n> prog [program args]
    options:
      -n <n> number of emulated PXNs
      -c <n> number of emulated cores per PXN
      -t <n> number of emulated harts per core
      -p <n> number of host threads
      -h     this help message
"
}

HOST_THREADS="${HOST_THREADS:-16}"
HOSTS="${HOSTS:-1}"
PROCS="${PROCS:-2}"
CORES="${CORES:-8}"
HARTS="${HARTS:-16}"
THREADD=$((${HOST_THREADS} * ${HOSTS} / ${PROCS}))
THREADS="${THREADS:-${THREADD}}"
# 8GB Main memory size by default
MAIN_MEMORY_SIZE="${MAIN_MEMORY_SIZE-8589934592}"
LAUNCH_DIR="${LAUNCH_DIR:-$(realpath ${DRV_ROOT}/../)}"
LAUNCH_SCRIPT="${LAUNCH_SCRIPT:-${LAUNCH_DIR}/pando-drv/tests/PANDOHammerDrvX.py}"

DBG=""

while getopts "p:n:c:t:hd" option; do
    case ${option} in
    p) # host threads
        HOST_THREADS=${OPTARG}
        ;;

    n) # number of emulated PXNs
        HOSTS=${OPTARG}
        ;;

    c) # number of emulated cores per PXN
        CORES=${OPTARG}
        ;;

    t) # number of emulated cores per PXN
        HARTS=${OPTARG}
        ;;

    d) # Run with gdb
        DBG="gdb --args"
        ;;

    h) # help
        show_help
        exit
        ;;
    esac
done

shift $(expr $OPTIND - 1)
PROG=$1
shift

${DBG} sst -n ${HOST_THREADS} \
    "${LAUNCH_SCRIPT}" -- \
    --with-command-processor="${PROG}" \
    --num-pxn=${PROCS} \
    --pod-cores=${CORES} \
    --core-threads=${HARTS} \
    --drvx-stack-in-l1sp \
    --pxn-dram-size=${MAIN_MEMORY_SIZE} \
    ${PROG} $@

<<comment
${DBG} sst -n ${HOST_THREADS} \
    --verbose \
    "${LAUNCH_SCRIPT}" -- \
    --with-command-processor="${PROG}" \
    --num-pxn=${PROCS} \
    --pod-cores=${CORES} \
    --core-threads=${HARTS} \
    --drvx-stack-in-l1sp \
    --pxn-dram-size=${MAIN_MEMORY_SIZE} \
    --perf-stats \
    --stats-load-level=5 \
    --stats-preallocated-phase=64 \
    ${PROG} $@
comment
