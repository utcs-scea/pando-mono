# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.

#/bin/bash
# Set up pando environment

mkdir -p install
export DRV_ROOT=$(realpath install)
export PATH=${PATH:-''}:$DRV_ROOT/bin
export PYTHONPATH=$(realpath tests):${PYTHONPATH:-''}
export LD_LIBRARY_PATH=$DRV_ROOT/lib
export LIBRARY_PATH=$DRV_ROOT/lib
export C_INCLUDE_PATH=$DRV_ROOT/include
export CPLUS_INCLUDE_PATH=$DRV_ROOT/include

# set drvx flag
export NO_DRVR=1
