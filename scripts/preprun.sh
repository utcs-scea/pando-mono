#!/bin/bash
# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#
# SPDX-License-Identifier: MIT
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
#
# Script for executing PANDO programs via PREP emulation using the GASNet smp-conduit.

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
      -h     this help message
"
}

#
# Main
#

harts=""

while getopts "n:c:t:h" option; do
    case ${option} in
    n) # number of emulated PXNs
        nodes=${OPTARG}
        ;;
    c) # number of emulated cores per PXN
        cores=${OPTARG}
        ;;
    t) # number of emulated cores per PXN
        harts=${OPTARG}
        ;;
    h) # help
        show_help
        exit
        ;;
    esac
done
shift $(expr $OPTIND - 1)
prog=$@

export GASNET_PSHM_NODES=$nodes
export PANDO_PREP_NUM_CORES=$cores

if [ -n "$harts" ]; then
    export PANDO_PREP_NUM_HARTS=$harts
fi

exec $prog
