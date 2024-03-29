# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

FROM ubuntu:22.04

ENV DRV_ROOT /install

RUN apt update -y \
        && apt install -y \
        make \
        build-essential \
        openssh-client \
        && apt clean -y

# install sst core
RUN apt update -y \
        && apt install -y \
        libopenmpi-dev \
        openmpi-bin \
        openmpi-common \
        libtool \
        libtool-bin \
        autoconf \
        python3 \
        python3-dev \
        automake \
        git \
        && git clone https://github.com/sstsimulator/sst-core.git /sst-core-src \
        && cd /sst-core-src \
        && ./autogen.sh \
        && mkdir -p /sst-core-build \
        && mkdir -p ${DRV_ROOT} \
        && cd /sst-core-build \
        && /sst-core-src/configure --prefix=${DRV_ROOT} \
        && make -j `nproc` install \
        && rm -rf /sst-core-src \
        && rm -rf /sst-core-build \
        && apt remove -y git automake \
        && apt clean -y


# install ramulator
RUN apt update -y \
        && apt install -y \
        git \
        clang \
        && git clone https://github.com/mrutt92/ramulator /root/sst-ramulator-src \
        && cd /root/sst-ramulator-src \
        && git checkout mrutt \
        && make -j `nproc` libramulator.so

# install dramsim3
RUN apt update -y \
        && apt install -y git clang \
        && git clone https://github.com/mrutt92/DRAMSim3 /root/sst-dramsim3-src \
        && cd /root/sst-dramsim3-src \
        && git checkout basejump_fixes \
        && make -j `nproc` libdramsim3.so

# install sst-elements
RUN apt update -y \
        && apt install -y \
        git \
        libltdl-dev \
        && git clone https://github.com/mrutt92/sst-elements.git /sst-elements-src \
        && cd /sst-elements-src \
        && git checkout devel-drv-changes \
        && ./autogen.sh \
        && mkdir -p /sst-elements-build \
        && cd /sst-elements-build \
        && /sst-elements-src/configure \
        --prefix=${DRV_ROOT} \
        --with-sst-core=${DRV_ROOT} \
        --with-ramulator=/root/sst-ramulator-src \
        --with-dramsim3=/root/sst-dramsim3-src \
        && make -j `nproc` install \
        && rm -rf /sst-elements-build \
        && apt remove -y git libltdl-dev \
        && apt clean -y

# install boost
RUN apt update -y \
        && apt install -y \
        wget \
        && mkdir -p /downloads \
        && cd /downloads \
        && wget https://boostorg.jfrog.io/artifactory/main/release/1.82.0/source/boost_1_82_0.tar.gz \
        && tar zxf boost_1_82_0.tar.gz \
        && cd /downloads/boost_1_82_0 \
        && /downloads/boost_1_82_0/bootstrap.sh \
        && /downloads/boost_1_82_0/b2 --prefix=${DRV_ROOT} install \
        && rm -rf /downloads \
        && apt remove -y wget \
        && apt clean -y


RUN mkdir ~/.ssh
ARG ssh_prv_key
ARG ssh_pub_key
RUN echo "$ssh_prv_key" > /root/.ssh/id_ed25519 && \
    echo "$ssh_pub_key" > /root/.ssh/id_ed25519.pub && \
    chmod 600 /root/.ssh/id_ed25519 && \
    chmod 600 /root/.ssh/id_ed25519.pub && \
    ssh-keyscan github.com >> /root/.ssh/known_hosts

RUN eval "$(ssh-agent -s)" && \
    ssh-add ~/.ssh/id_ed25519

RUN apt update -y \
        && apt install -y git automake gawk wget curl texinfo libgmp-dev flex bison \
        && git clone git@github.com:AMDResearch/pando-riscv-gnu-toolchain /riscv-gnu-toolchain \
        && cd /riscv-gnu-toolchain \
        && git checkout pandodrvsim \
        && ./configure --enable-multilib --with-arch=rv64imafd --disable-linux --prefix=${DRV_ROOT} \
        && make -j `nproc` \
        && make install \
        && rm -rf /riscv-gnu-toolchain \
        && apt remove -y git automake gawk wget curl texinfo libgmp-dev flex bison \
        && apt clean -y

# install other dependencies
RUN apt update -y \
        && apt install -y \
        git \
        clang \
        libopenmpi-dev \
        bc \
        python3-pip \
        wget \
        && apt clean -y

RUN pip3 install gdown

ENV PATH "${PATH}":${DRV_ROOT}/bin
ENV PYTHONPATH "/root/drv/tests:${PYTHONPATH}"
ENV LD_LIBRARY_PATH ${DRV_ROOT}/lib
ENV LIBRARY_PATH ${DRV_ROOT}/lib
ENV C_INCLUDE_PATH ${DRV_ROOT}/include
ENV CPLUS_INCLUDE_PATH ${DRV_ROOT}/include
ENV OMPI_CXX clang++

# copy drv sources
COPY  . /root/drv/
WORKDIR /root/drv/

# build drv
RUN cd /root/drv \
    && make -j `nproc` install \
    && make -j `nproc` examples
RUN cd /root/drv
