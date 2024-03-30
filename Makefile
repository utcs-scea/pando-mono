# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

SHELL := /bin/bash

UNAME ?= $(shell whoami)
UID ?= $(shell id -u)
GID ?= $(shell id -g)

BASE_IMAGE_NAME ?= pando-galois
IMAGE_NAME ?= ${UNAME}-${BASE_IMAGE_NAME}
SRC_DIR ?= $(shell pwd)
VERSION ?= $(shell git log --pretty="%h" -1 Dockerfile.dev)
DRIVEX_VERSION ?= $(shell cd pando-drv && git log --pretty="%h" -1)

CONTAINER_SRC_DIR ?= /pando
CONTAINER_BUILD_DIR ?= /pando/dockerbuild
CONTAINER_WORKDIR ?= ${CONTAINER_SRC_DIR}
CONTAINER_CONTEXT ?= default
CONTAINER_OPTS ?=
CONTAINER_CMD ?= setarch `uname -m` -R bash -l
INTERACTIVE ?= i

BUILD_TYPE ?= Release

# CMake variables
PANDO_TEST_DISCOVERY_TIMEOUT ?= 150
PANDO_EXTRA_CXX_FLAGS ?= ""
PANDO_BUILD_DOCS ?= OFF

# Drive X variables
DRV_BUILD_DIR ?= /pando/dockerbuild-drv
DRV_ROOT ?=

# Developer variables that should be set as env vars in startup files like .profile
PANDO_CONTAINER_MOUNTS ?=
PANDO_CONTAINER_ENV ?=

.PHONY: docker docker-image-dependencies

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

ci-image:
	@${MAKE} docker-image-dependencies
	@docker --context ${CONTAINER_CONTEXT} build \
	--build-arg SRC_DIR=${CONTAINER_SRC_DIR} \
	--build-arg BUILD_DIR=${CONTAINER_BUILD_DIR} \
	--build-arg UNAME=runner \
  --build-arg UID=1078 \
  --build-arg GID=504 \
	--build-arg DRIVEX_IMAGE=drivex:${DRIVEX_VERSION} \
	-t pando:${VERSION} \
	--file Dockerfile.dev \
	--target dev .

docker-image:
	@${MAKE} docker-image-dependencies
	@docker --context ${CONTAINER_CONTEXT} build \
	--build-arg SRC_DIR=${CONTAINER_SRC_DIR} \
	--build-arg BUILD_DIR=${CONTAINER_BUILD_DIR} \
	--build-arg UNAME=${UNAME} \
	--build-arg IS_CI=false \
  --build-arg UID=${UID} \
  --build-arg GID=${GID} \
	--build-arg DRIVEX_IMAGE=drivex:${DRIVEX_VERSION} \
	-t ${IMAGE_NAME}:${VERSION} \
	--file Dockerfile.dev \
	--target dev .

docker-image-dependencies:
	@mkdir -p dockerbuild
	@mkdir -p data
	@docker image inspect pando-gasnet:latest >/dev/null 2>&1 || \
	docker --context ${CONTAINER_CONTEXT} build \
	-t pando-gasnet:latest \
	--file Dockerfile.dev \
	--target gasnet .
	@docker image inspect drivex:${DRIVEX_VERSION} >/dev/null 2>&1 || \
	docker --context ${CONTAINER_CONTEXT} build \
	--build-arg GASNET_IMAGE=pando-gasnet:latest \
	-t drivex:${DRIVEX_VERSION} \
	--file Dockerfile.dev \
	--target drivex .

docker:
	@docker --context ${CONTAINER_CONTEXT} run --rm \
	-v ${SRC_DIR}/:${CONTAINER_SRC_DIR} \
	${PANDO_CONTAINER_MOUNTS} \
	${PANDO_CONTAINER_ENV} \
	--privileged \
	--workdir=${CONTAINER_WORKDIR} ${CONTAINER_OPTS} -${INTERACTIVE}t \
	${IMAGE_NAME}:${VERSION} \
	${CONTAINER_CMD}

cmake-mpi:
	@echo "Must be run from inside the dev Docker container"
	@. /dependencies/spack/share/spack/setup-env.sh && \
	spack load gasnet@${GASNET_VERSION}%gcc@${GCC_VERSION} qthreads@${QTHEADS_VERSION}%gcc@${GCC_VERSION} openmpi@${OMPI_VERSION}%gcc@${GCC_VERSION} && \
	cmake \
  -S ${SRC_DIR} \
  -B ${BUILD_DIR} \
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
	-DCMAKE_CXX_FLAGS=${PANDO_EXTRA_CXX_FLAGS} \
	-DPANDO_PREP_GASNET_CONDUIT=mpi \
  -DCMAKE_INSTALL_PREFIX=/opt/pando-lib-galois \
  -DBUILD_TESTING=ON \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_DOCS=${PANDO_BUILD_DOCS} \
	-DPANDO_TEST_DISCOVERY_TIMEOUT=${PANDO_TEST_DISCOVERY_TIMEOUT} \
	-DCMAKE_CXX_COMPILER=g++-12 \
  -DCMAKE_C_COMPILER=gcc-12

cmake-smp:
	@echo "Must be run from inside the dev Docker container"
	@. /dependencies/spack/share/spack/setup-env.sh && \
	spack load gasnet@${GASNET_VERSION}%gcc@${GCC_VERSION} qthreads@${QTHEADS_VERSION}%gcc@${GCC_VERSION} openmpi@${OMPI_VERSION}%gcc@${GCC_VERSION} && \
	cmake \
  -S ${SRC_DIR} \
  -B ${BUILD_DIR}-smp\
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
	-DCMAKE_CXX_FLAGS=${PANDO_EXTRA_CXX_FLAGS} \
	-DPANDO_PREP_GASNET_CONDUIT=smp \
  -DCMAKE_INSTALL_PREFIX=/opt/pando-lib-galois \
  -DBUILD_TESTING=ON \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_DOCS=${PANDO_BUILD_DOCS} \
	-DPANDO_TEST_DISCOVERY_TIMEOUT=${PANDO_TEST_DISCOVERY_TIMEOUT} \
	-DCMAKE_CXX_COMPILER=g++-12 \
  -DCMAKE_C_COMPILER=gcc-12

cmake-drv:
	@cmake \
	-S ${SRC_DIR} \
	-B ${DRV_BUILD_DIR} \
	-DDRVX_ROOT=${DRV_ROOT} \
	-DPANDO_RT_BACKEND=DRVX \
	-DCMAKE_CXX_FLAGS=${PANDO_EXTRA_CXX_FLAGS} \
	-DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
	-DCMAKE_INSTALL_PREFIX=/opt/pando-lib-galois \
	-DBUILD_TESTING=ON \
	-DBUILD_EXAMPLES=ON \
	-DBUILD_DOCS=OFF \
	-DCMAKE_CXX_COMPILER=g++-12 \
  -DCMAKE_C_COMPILER=gcc-12

setup-ci: cmake-mpi

setup: cmake-mpi cmake-smp

drive-deps:
	@mkdir -p deps/
	@git clone --branch pando-rt-backend git@github.com:AMDResearch/pando-sst-core.git deps/sst-core-src
	@git clone --branch pando-rt-backend git@github.com:AMDResearch/pando-sst-elements.git deps/sst-elements-src

run-tests-mpi:
	set -o pipefail && \
	. ~/.profile && \
	cd ${CONTAINER_BUILD_DIR} && ctest --verbose | tee test.out && \
	! grep -E "Failure" test.out && ! grep -E "runtime error" test.out

run-tests-smp:
	set -o pipefail && \
	. ~/.profile && \
	cd ${CONTAINER_BUILD_DIR}-smp && ctest --verbose | tee test.out && \
	! grep -E "Failure" test.out && ! grep -E "runtime error" test.out

run-tests-drv:
	set -o pipefail && \
	. ~/.profile && \
	cd ${DRV_BUILD_DIR} && ctest --verbose | tee test.out && \
	! grep -E "Failure" test.out && ! grep -E "runtime error" test.out

run-tests: run-tests-mpi

# this command is slow since hooks are not stored in the container image
# this is mostly for CI use
docker-pre-commit:
	@docker --context ${CONTAINER_CONTEXT} run --rm \
	-v ${SRC_DIR}/:${CONTAINER_SRC_DIR} --privileged \
	--workdir=${CONTAINER_WORKDIR} -t \
	${IMAGE_NAME}:${VERSION} bash -lc "git config --global --add safe.directory /pando && make hooks && make pre-commit"
