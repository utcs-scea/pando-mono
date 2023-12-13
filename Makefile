# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

SHELL := /bin/bash

IMAGE_NAME ?= pando-galois
SRC_DIR ?= $(shell pwd)
GALOIS_VERSION ?= $(shell git log --pretty="%h" -1 Dockerfile.dev)
ROOT_IMAGE_VERSION ?= $(shell cd pando-rt/ && git log --pretty="%h" -1 docker)
VERSION ?= ${GALOIS_VERSION}-${ROOT_IMAGE_VERSION}

CONTAINER_SRC_DIR ?= /pando
CONTAINER_BUILD_DIR ?= /pando/dockerbuild
CONTAINER_WORKDIR ?= ${CONTAINER_SRC_DIR}
CONTAINER_CONTEXT ?= default
CONTAINER_OPTS ?=
CONTAINER_CMD ?= setarch `uname -m` -R bash -l
INTERACTIVE ?= i
UNAME ?= $(shell whoami)
UID ?= $(shell id -u)
GID ?= $(shell id -g)

BUILD_TYPE ?= Release

# CMake variables
PANDO_TEST_DISCOVERY_TIMEOUT ?= 150
PANDO_EXTRA_CXX_FLAGS ?= ""
PANDO_BUILD_DOCS ?= OFF

# Developer variables that should be set as env vars in startup files like .profile
PANDO_CONTAINER_MOUNTS ?=
PANDO_CONTAINER_ENV ?=

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
	@mkdir -p data
	@docker --context ${CONTAINER_CONTEXT} build \
	--build-arg SRC_DIR=${CONTAINER_SRC_DIR} \
	--build-arg BUILD_DIR=${CONTAINER_BUILD_DIR} \
  -t pando-rt:latest \
	--file pando-rt/docker/Dockerfile \
	./pando-rt/docker
	@docker --context ${CONTAINER_CONTEXT} build \
	--build-arg SRC_DIR=${CONTAINER_SRC_DIR} \
	--build-arg BUILD_DIR=${CONTAINER_BUILD_DIR} \
	--build-arg UNAME=${UNAME} \
  --build-arg UID=${UID} \
  --build-arg GID=${GID} \
	-t ${IMAGE_NAME}:${VERSION} \
	--file Dockerfile.dev \
	--target dev .

docker:
	@docker --context ${CONTAINER_CONTEXT} run --rm \
	-v ${SRC_DIR}/:${CONTAINER_SRC_DIR} \
	${PANDO_CONTAINER_MOUNTS} \
	${PANDO_CONTAINER_ENV} \
	--privileged \
	--workdir=${CONTAINER_WORKDIR} ${CONTAINER_OPTS} -${INTERACTIVE}t \
	${IMAGE_NAME}:${VERSION} \
	${CONTAINER_CMD}

setup:
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
  -DCMAKE_C_COMPILER=gcc-12 && \
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

run-tests:
	. /dependencies/spack/share/spack/setup-env.sh && \
	spack load gasnet qthreads openmpi && \
	cd ${BUILD_DIR} && ctest --output-on-failure

# this command is slow since hooks are not stored in the container image
# this is mostly for CI use
docker-pre-commit:
	@docker --context ${CONTAINER_CONTEXT} run --rm \
	-v ${SRC_DIR}/:${CONTAINER_SRC_DIR} --privileged \
	--workdir=${CONTAINER_WORKDIR} -t \
	${IMAGE_NAME}:${VERSION} bash -lc "make hooks && make pre-commit"
