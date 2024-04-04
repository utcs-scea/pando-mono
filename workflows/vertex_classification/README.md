<!--
  ~ SPDX-License-Identifier: MIT
  ~ Copyright (c) 2023. University of Texas at Austin. All rights reserved.
  -->

# Pando Workflow 1 Galois

This repo submodules the PANDO Galois library and uses it to implement wf1.
wf1 is graph neural network (GNN), and includes vertex classification,
link prediction, and multi-hop reasoning.

Quick Setup:

```shell
git submodule update --init --recursive
make dependencies
make hooks
make docker-image
make docker
# These commands are run in the container `make docker` drops you into
make setup
make -C dockerbuild -j8
make run-tests
```

Developers can run a hello-world smoke test inside their containers by running
`bash scripts/run.sh`.

Developers can run multi-hop reasoning inside their containers with `bash scripts/run_mhr.sh`.

## Tools

### [asdf](https://asdf-vm.com)

Provides a declarative set of tools pinned to
specific versions for environmental consistency.

These tools are defined in `.tool-versions`.
Run `make dependencies` to initialize a new environment.

### [pre-commit](https://pre-commit.com)

A left shifting tool to consistently run a set of checks on the code repo.
Our checks enforce syntax validations and formatting.
We encourage contributors to use pre-commit hooks.

```shell
# install all pre-commit hooks
make hooks

# run pre-commit on repo once
make pre-commit
```

### Further tooling

For tooling used by `pando-rt` see their
[docs](https://github.com/AMDResearch/pando-rt/blob/main/docs/developer.md).
This includes `clang-format` and `cpplint`.

### [PANDO Runtime System](https://amdresearch.github.io/pando-rt)

Please see [pando-rt's README](https://github.com/AMDResearch/pando-rt/blob/main/README.md).

## Build

```shell
# Create build directory
mkdir build
cd build
# Create CMake file
cmake ${SRC_DIR}
# Build codes
make -j
```

## Running

We provide a script to run the vertex classification.
Users can configure an execution through parameters.
First, users should provide the source and build directories.

```shell
export SRC_DIR=/home/../PANDO-wf1-gal-root
export BUILD_DIR=/home/../PANDO-wf1-gal-root.build
```

Then, users can run vertex classification through the below command with
default parameters:
2 PXNs, 4 cores, 16 harts, 32KB L1SP, 16GB L2SP, 96GB main memory, 100 epochs,
data.small.csv which has about 200 vertices.

### PREP

For PREP, we support a script, `run_wf1.sh`.

```shell
source ${SRC_DIR}/scripts/run_wf1.sh
```

Users can set/use environmental parameters through environment variables.

```shell
HOSTS={# of PXNs} CORES={# of cores} HARTS={# of harts}                   \
STACK_SIZE={L1SP size; this should be >= 13000}                           \
POD_MEMORY_SIZE={L2SP size} MAIN_MEMORY_SIZE={Main memory size}           \
EPOCHS={# of epochs}                                                      \
EXE={binary name; so ${BUILD_DIR}/src/${EXE} should point to the binary}  \
GRAPHFILE={Input WMD graph path} LOGFILE={Log file path}                  \
ACCURACYFILE={Path of the log file that only logs accuracy per epoch}     \
source ${SRC_DIR}/scripts/run_wf1.sh
```

For example,

```shell
HOSTS=2 CORES=2 HARTS=16 STACK_SIZE=13000 EPOCHS=10 \
GRAPHFILE=/blah/blah/data0.0001.csv source          \
${SRC_DIR}/scripts/run_wf1.sh
```

Note that as it is mentioned in the above, the current vertex classification
requires more than 13000B L1SP memory.

All the logs are redirected to $LOGFILE.
Accuracy per epoch is redirected to $ACCURACYFILE.

#### Disabling Kernels

It is possible to disable a kernel 2.
This can be done by setting `DISABLE_K2=0` when using `scripts/run_wf1.sh`.

### DRV-X

You can WF1 vertex classification on DRV-X with the same environment with PREP through
the following commands:

```shell
sst -n 1 ~/pando/pando-drv/tests/PANDOHammerDrvX.py --  \
 --core-threads=16 --with-command-processor=./libwf1.so \
 --num-pxn=2 --pod-cores=2 --drvx-stack-in-l1sp \
 ./libwf1.so \
 -g /blah/blah/data0.0001.csv -e 10 >& drvx-wf1k2-8-.data0.0001.out

```

## WF1 deliveries

Vertex classification that matches to AGILE WF1 vertex classification

* Ego graph construction
* Vertex embedding dropout (rate = 0.5)
* Minibatching (size = 128)
* Graph convolutional network
* ReLU activation
* Softmax
* Adam optimizer

## Commands to run WF1 in SST inside an Application apptainer

* Command for single-PXN
sst -n 1 /root/pando-src/pando-drv/tests/PANDOHammerDrvX.py -- --core-threads=1 \
  --with-command-processor=./src/libwf1.so --num-pxn=1 --pod-cores=2 \
  --drvx-stack-in-l1sp ./src/libwf1.so -g ../inputs/data0.0001.csv -e 10
* Command for multi-PXN
sst -n 1 /root/pando-src/pando-drv/tests/PANDOHammerDrvX.py -- --core-threads=1 \
  --with-command-processor=./src/libwf1.so --num-pxn=2 --pod-cores=2 \
  --drvx-stack-in-l1sp ./src/libwf1.so -g ../inputs/data0.0001.csv -e 10

### Multi-Hop Reasoning

Multi-Hop Reasoning is run from a separate executable and make target `mhr`.

It can be run using the script `scripts/run_mhr.sh`.

### Link Prediction

Development of Link Prediction is in progress.
Currently, the link prediction code performs a forward propagation using
layers of GCN followed by a dot product and a sigmoid layer.
The final sigmoid layer is used as a prediction for LP.

Then, users can run link prediction through the below command with
default parameters:
2 PXNs, 4 cores, 16 harts, 32KB L1SP, 16GB L2SP, 96GB main memory, 100 epochs,
data.small.csv which has about 200 vertices.

```shell
source ${SRC_DIR}/scripts/run_wf1_lp.sh
```
