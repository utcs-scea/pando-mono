# SPDX-License-Identifier: MIT
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.

#/bin/bash
# Build dependencies for DrvX
set -e

mkdir -p deps

cd deps

if [ ! -d sst-core-src ]; then
    git clone --branch pando-rt-backend git@github.com:AMDResearch/pando-sst-core.git sst-core-src
else
    cd sst-core-src
    git pull
    cd ..
fi
cd sst-core-src &&
    ./autogen.sh &&
    mkdir -p sst-core-build &&
    cd sst-core-build &&
    ../configure --prefix=$DRV_ROOT &&
    make -j $(nproc) install &&
    cd ../..

if [ ! -d sst-elements-src ]; then
    git clone --branch pando-rt-backend git@github.com:AMDResearch/pando-sst-elements.git sst-elements-src
else
    cd sst-elements-src
    git pull
    cd ..
fi
cd sst-elements-src &&
    ./autogen.sh &&
    mkdir -p sst-elements-build &&
    cd sst-elements-build &&
    ../configure --prefix=$DRV_ROOT --with-sst-core=$DRV_ROOT &&
    make -j $(nproc) install &&
    cd ../..

if [ ! -d boost_1_82_0 ]; then
    wget https://boostorg.jfrog.io/artifactory/main/release/1.82.0/source/boost_1_82_0.tar.gz &&
        tar zxf boost_1_82_0.tar.gz
fi
cd boost_1_82_0 &&
    ./bootstrap.sh &&
    ./b2 --prefix=$DRV_ROOT install &&
    cd ..

cd ..
