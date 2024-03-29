# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.

. ${HOME}/spack_env
make -C $BUILD_DIR -j`nproc`
