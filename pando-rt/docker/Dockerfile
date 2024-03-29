# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
FROM ubuntu:22.04

ARG COMPILER=g++-12
ARG GCC_VERSION=12.3.0
ARG QTHEADS_VERSION=1.16
ARG GASNET_VERSION=2023.3.0
ARG OMPI_VERSION=4.1.5
ARG SRC_DIR=/pando-rt
ARG BUILD_DIR=/pando-rt/dockerbuild


RUN apt update -y \
    &&  apt install -y \
    git \
    python3 \
    python3-pip \
    language-pack-en\
    pkg-config\
    ${COMPILER}=${GCC_VERSION}* \
    gfortran-12 \
    &&  apt clean \
    &&  update-locale

# CMake
RUN apt install -y ca-certificates gpg wget \
&&  wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null \
&&  echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | tee /etc/apt/sources.list.d/kitware.list >/dev/null \
&&  apt-get update -y && rm /usr/share/keyrings/kitware-archive-keyring.gpg && apt-get install -y kitware-archive-keyring \
&&  apt-get update -y \
&&  apt-get install -y cmake \
&&  apt-get purge -y gpg wget \
&&  apt-get clean -y \
&&  rm -rf /var/lib/apt/lists/*


WORKDIR /dependencies

RUN git clone --depth 1 --branch v0.20.1 https://github.com/spack/spack.git

RUN . /dependencies/spack/share/spack/setup-env.sh \
&&  spack compiler find \
&&  spack install -j`nproc` qthreads@${QTHEADS_VERSION}%gcc@${GCC_VERSION} target=x86_64 \
                            openmpi@${OMPI_VERSION}%gcc@${GCC_VERSION} target=x86_64 \
&&  spack load openmpi@${OMPI_VERSION}%gcc@${GCC_VERSION} \
&&  spack install -j`nproc` gasnet@${GASNET_VERSION}%gcc@${GCC_VERSION} conduits=smp,mpi target=x86_64

RUN echo ". /dependencies/spack/share/spack/setup-env.sh" >> /root/spack_env \
    &&  echo "spack load qthreads openmpi gasnet" >> /root/spack_env

ENV SRC_DIR=${SRC_DIR}
ENV BUILD_DIR=${BUILD_DIR}
ENV GCC_VERSION=${GCC_VERSION}
ENV QTHEADS_VERSION=${QTHEADS_VERSION}
ENV GASNET_VERSION=${GASNET_VERSION}
ENV OMPI_VERSION=${OMPI_VERSION}
ENV TERM=xterm-256color
ENV USER=${UNAME}
ENV LANG="C.UTF-8"
ENV LC_COLLATE="C.UTF-8"
ENV LC_CTYPE="C.UTF-8"
ENV LC_MESSAGES="C.UTF-8"
ENV LC_MONETARY="C.UTF-8"
ENV LC_NUMERIC="C.UTF-8"
ENV LC_TIME="C.UTF-8"
WORKDIR ${SRC_DIR}
