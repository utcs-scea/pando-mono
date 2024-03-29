#!/bin/bash
#
#
# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.

. ${HOME}/spack_env
cd $BUILD_DIR
ctest -V -N -O pando-rt-ctest-$GITHUB_JOB-commands.log
ctest -V --output-log pando-rt-ctest-$GITHUB_JOB.log --stop-on-failure
