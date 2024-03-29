#!/bin/bash
#
#
# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.

if [[ `cat /proc/sys/kernel/randomize_va_space` != 0 ]]; then
  echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
fi

TERMINAL=${TERMINAL:-/bin/bash}
EXTRA_ARGS=${EXTRA_ARGS:-"-v $HOME/.vim:/home/$(whoami)/.vim"}
docker run -ti --rm -v $(pwd):/pando-rt ${EXTRA_ARGS} pando-rt-dev:latest ${TERMINAL}
