#!/bin/bash
#
#
# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.

. ${HOME}/spack_env
cmake \
  -S $SRC_DIR \
  -B $BUILD_DIR \
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
  -DCMAKE_INSTALL_PREFIX=/opt/pando-rt \
  -DBUILD_TESTING=ON \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_DOCS=OFF \
  -DPANDO_RT_BACKEND=PREP \
  -DPANDO_PREP_GASNET_CONDUIT=mpi
