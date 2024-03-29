#!/bin/bash
#
#
# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.

set -o errexit
set -o nounset
set -o pipefail

BUILD_TYPE=${BUILD_TYPE:-Release}

mkdir -p dockerbuild
docker build \
  -t pando-rt:latest ./docker

docker build \
  --build-arg "UNAME=$(whoami)" \
  --build-arg "UID=$(id -u)" \
  --build-arg "GID=$(id -g)" \
  -t pando-rt-dev:latest -f ./docker/Dockerfile.dev ./docker

docker run \
  -ti \
  --rm \
  -e "BUILD_TYPE=${BUILD_TYPE}" \
  -v $(pwd):/pando-rt \
  pando-rt-dev:latest \
  /bin/bash /home/$(whoami)/setup.sh
