#!/bin/bash
#
# SPDX-License-Identifier: MIT
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
#
# Driver script for executing PANDO programs via DRV-X simulation.


set -o errexit
set -o nounset
set -o pipefail

#
# Help function
#
function show_help() {
    echo "
usage: preprun -n <n> prog.so [program args]
    options:
      -n <n> number of emulated PXNs
      -c <n> number of emulated cores per PXN
      -s <n> number of threads to be used by SST
      -h     this help message
"
}

#
# Main
#

# Defaults
sst_threads=1

while getopts "n:c:s:h" option
do
    case ${option} in
        n) # number of emulated PXNs
            nodes=${OPTARG}
            ;;
        c) # number of emulated cores per PXN
            cores=${OPTARG}
            ;;
        s) # number of threads to be used by SST
            sst_threads=${OPTARG}
            ;;
        h) # help
            show_help
            exit
            ;;
    esac
done
shift $(expr $OPTIND - 1 )

prog=$1
prog_with_args=$@

# setup DRV-related env vars
DRV_DIR=${DRV_ROOT:-/dependencies/install}/../pando-drv/
echo "Using DRV dir: ${DRV_DIR}. To change the DRV dir, set DRV_ROOT variable"
PWD_DIR=`pwd`
cd $DRV_DIR/
. ./load_drvx.sh
cd $PWD_DIR

# execute with SST/DRV
# echo exec sst -n $sst_threads $DRV_DIR/../tests/PANDOHammerDrvX.py -- --with-command-processor=$prog --num-pxn=$nodes --pod-cores=$cores --drvx-stack-in-l1sp $prog_with_args
exec sst -n $sst_threads $DRV_DIR/tests/PANDOHammerDrvX.py -- --with-command-processor=$prog --num-pxn=$nodes --pod-cores=$cores --drvx-stack-in-l1sp $prog_with_args
