# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

SHELL := /bin/bash

UNAME ?= $(shell whoami)
IMAGE_NAME ?= ${UNAME}-pando-wf1
VERSION ?= latest
LIB_IMAGE_NAME ?= ${UNAME}-pando-galois
LIB_VERSION ?= $(shell cd Galois && git log --pretty="%h" -1 Dockerfile.dev)
SRC_DIR ?= $(shell pwd)

CONTAINER_SRC_DIR ?= /pando
CONTAINER_BUILD_DIR ?= /pando/dockerbuild
CONTAINER_WORKDIR ?= ${CONTAINER_SRC_DIR}
CONTAINER_CONTEXT ?= default
CONTAINER_OPTS ?=
CONTAINER_CMD ?= setarch `uname -m` -R bash -l
INTERACTIVE ?= i

BUILD_TYPE ?= Release

dependencies: dependencies-asdf

dependencies-asdf:
	@echo "Updating asdf plugins..."
	@asdf plugin update --all >/dev/null 2>&1 || true
	@echo "Adding new asdf plugins..."
	@cut -d" " -f1 ./.tool-versions | xargs -I % asdf plugin-add % >/dev/null 2>&1 || true
	@echo "Installing asdf tools..."
	@cat ./.tool-versions | xargs -I{} bash -c 'asdf install {}'
	@echo "Updating local environment to use proper tool versions..."
	@cat ./.tool-versions | xargs -I{} bash -c 'asdf local {}'
	@asdf reshim
	@echo "Done!"

hooks:
	@pre-commit install --hook-type pre-commit
	@pre-commit install-hooks

pre-commit:
	@pre-commit run -a

git-submodules:
	@git submodule update --init --recursive

docker-image:
	@mkdir -p dockerbuild
	@docker image inspect ${LIB_IMAGE_NAME}:${LIB_VERSION} >/dev/null 2>&1 || ${MAKE} -C Galois docker-image
	@docker --context ${CONTAINER_CONTEXT} build \
	-t ${IMAGE_NAME}:${VERSION} \
	--build-arg BUILD_IMAGE=${LIB_IMAGE_NAME}:${LIB_VERSION} \
	--file Dockerfile.dev \
	--target dev .

docker:
	@docker --context ${CONTAINER_CONTEXT} run --rm \
	-v ${SRC_DIR}/:${CONTAINER_SRC_DIR} --privileged \
	--workdir=${CONTAINER_WORKDIR} ${CONTAINER_OPTS} -${INTERACTIVE}t \
	${IMAGE_NAME}:${VERSION} ${CONTAINER_CMD}

setup:
	@echo "Must be run from inside the dev Docker container"
	@. /dependencies/spack/share/spack/setup-env.sh && \
	spack load gasnet@${GASNET_VERSION}%gcc@${GCC_VERSION} qthreads@${QTHEADS_VERSION}%gcc@${GCC_VERSION} openmpi@${OMPI_VERSION}%gcc@${GCC_VERSION} && \
	cmake \
  -S ${SRC_DIR} \
  -B ${BUILD_DIR} \
	-DCMAKE_CXX_FLAGS=${PANDO_EXTRA_CXX_FLAGS} \
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
  -DCMAKE_INSTALL_PREFIX=/opt/pando-wf1 \
  -DBUILD_TESTING=ON \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_DOCS=OFF && \
	cmake \
  -S ${SRC_DIR} \
  -B ${BUILD_DIR}-mpi \
	-DCMAKE_C_COMPILER=gcc-12 \
	-DCMAKE_CXX_COMPILER=g++-12 \
	-DPANDO_PREP_GASNET_CONDUIT=mpi \
	-DCMAKE_CXX_FLAGS=${PANDO_EXTRA_CXX_FLAGS} \
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
  -DCMAKE_INSTALL_PREFIX=/opt/pando-wf1 \
  -DBUILD_TESTING=ON \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_DOCS=OFF

run-tests:
	@cd ${BUILD_DIR} && ctest --output-on-failure

# this command is slow since hooks are not stored in the container image
# this is mostly for CI use
docker-pre-commit:
	@docker --context ${CONTAINER_CONTEXT} run --rm \
	-v ${SRC_DIR}/:${CONTAINER_SRC_DIR} --privileged \
	--workdir=${CONTAINER_WORKDIR} -t \
	${IMAGE_NAME}:${VERSION} bash -lc "make hooks && make pre-commit"
