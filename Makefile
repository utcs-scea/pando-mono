# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

SHELL := /bin/bash

IMAGE_NAME ?= pando-galois
VERSION ?= latest
SRC_DIR ?= $(shell pwd)

CONTAINER_SRC_DIR ?= /pando
CONTAINER_BUILD_DIR ?= /pando/dockerbuild
CONTAINER_WORKDIR ?= ${CONTAINER_SRC_DIR}
CONTAINER_CONTEXT ?= default
CONTAINER_OPTS ?=
CONTAINER_CMD ?= bash -l

BUILD_TYPE ?= Release

VIM ?= -v /home/${USER}/.vim/:/home/${USER}/.vim

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
	@docker --context ${CONTAINER_CONTEXT} build \
	--build-arg SRC_DIR=${CONTAINER_SRC_DIR} \
	--build-arg BUILD_DIR=${CONTAINER_BUILD_DIR} \
  -t pando-rt:latest \
	--file pando-rt/docker/Dockerfile \
	./pando-rt/docker
	@docker --context ${CONTAINER_CONTEXT} build \
	--build-arg SRC_DIR=${CONTAINER_SRC_DIR} \
	--build-arg BUILD_DIR=${CONTAINER_BUILD_DIR} \
	--build-arg UNAME=$(shell whoami) \
  --build-arg UID=$(shell id -u) \
  --build-arg GID=$(shell id -g) \
	-t ${IMAGE_NAME}:${VERSION} \
	--file Dockerfile.dev \
	--target dev .

docker:
	@docker --context ${CONTAINER_CONTEXT} run --rm \
	-v ${SRC_DIR}/:${CONTAINER_SRC_DIR} \
	${VIM} \
	--privileged \
	--workdir=${CONTAINER_WORKDIR} ${CONTAINER_OPTS} -it \
	${IMAGE_NAME}:${VERSION} ${CONTAINER_CMD}

setup:
	@echo "Must be run from inside the dev Docker container"
	@. /dependencies/spack/share/spack/setup-env.sh && \
	spack load gasnet@${GASNET_VERSION}%gcc@${GCC_VERSION} qthreads@${QTHEADS_VERSION}%gcc@${GCC_VERSION} && \
	cmake \
  -S ${SRC_DIR} \
  -B ${BUILD_DIR} \
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
  -DCMAKE_INSTALL_PREFIX=/opt/pando-lib-galois \
  -DBUILD_TESTING=ON \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_DOCS=OFF

run-tests:
	@cd ${BUILD_DIR} && ctest

# this command is slow since hooks are not stored in the container image
# this is mostly for CI use
docker-pre-commit:
	@docker --context ${CONTAINER_CONTEXT} run --rm \
	-v ${SRC_DIR}/:${CONTAINER_SRC_DIR} --privileged \
	--workdir=${CONTAINER_WORKDIR} -it \
	${IMAGE_NAME}:${VERSION} bash -lc "make hooks && make pre-commit"
