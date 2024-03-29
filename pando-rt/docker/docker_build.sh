#!/bin/bash
#
#
# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.

set -o errexit
set -o nounset
set -o pipefail

docker run -ti --rm -v $(pwd):/pando-rt pando-rt-dev:latest /bin/bash /home/$(whoami)/build.sh
