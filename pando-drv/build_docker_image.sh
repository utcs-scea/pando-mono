# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington
docker build . -t pando-drv \
    --build-arg ssh_prv_key="$(cat ~/.ssh/id_rsa)" \
    --build-arg ssh_pub_key="$(cat ~/.ssh/id_rsa.pub)" \
    -f ./docker/Dockerfile
