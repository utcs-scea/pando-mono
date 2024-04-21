#/bin/bash
# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.

# Build dependencies for DrvX
set -eo pipefail

echo DRV_ROOT=$DRV_ROOT
ls -al $DRV_ROOT

pushd deps

if [ ! -d sst-core-src ]; then
    echo "Run 'make drive-deps' outside of docker first"
    pwd
    ls -al
    exit 1
fi

pushd sst-core-src &&
    ./autogen.sh &&
    mkdir -p sst-core-build &&
    pushd sst-core-build &&
    ../configure --prefix=$DRV_ROOT &&
    make -j $(nproc) install || exit 1
popd
popd
echo DRV_ROOT=$DRV_ROOT
ls -al $DRV_ROOT

if [ ! -d sst-elements-src ]; then
    echo "Run 'make drive-deps' outside of docker first"
    exit 1
fi
pushd sst-elements-src &&
    ./autogen.sh &&
    mkdir -p sst-elements-build &&
    pushd sst-elements-build &&
    ../configure --prefix=$DRV_ROOT --with-sst-core=$DRV_ROOT &&
    make -j $(nproc) install || exit 1
popd
popd

echo DRV_ROOT=$DRV_ROOT
ls -al $DRV_ROOT

if [ ! -d boost_1_82_0 ]; then
    wget https://boostorg.jfrog.io/artifactory/main/release/1.82.0/source/boost_1_82_0.tar.gz &&
        tar zxf boost_1_82_0.tar.gz
fi

pushd boost_1_82_0 &&
    ./bootstrap.sh &&
    ./b2 --prefix=$DRV_ROOT install || exit 1

popd
popd

echo DRV_ROOT=$DRV_ROOT
ls -al $DRV_ROOT
