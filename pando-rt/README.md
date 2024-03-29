# PANDO Runtime System

The [PANDO Runtime System](https://amdresearch.github.io/pando-rt) (pando-rt) abstracts both emulation platforms and the PANDO hardware to allow an easy development of applications targeting the PANDO system.

It consists of ROOT, as set of Application Programming Interfaces (APIs) for developing applications, and backends, internal APIs that target emulation platforms, simulators and the PANDO hardware.

User code is expected to use the ROOT API to develop applications and avoid backend APIs to maximize portability.

## Application Entry Point 

Applications are expected to define/implement a special function called `pandoMain` that accepts the typical C/C++-style `argc`/`argv` command line arguments. Each backend will have its own driver application that kickstarts the PANDO runtime and invokes the user-defined `pandoMain` function.

## Prerequisites

pando-rt requires:
* Compilers: GCC 10.x, 11.x, 12.x (others may work, but have not been tested)
* [CMake](https://cmake.org/) 3.24+

## Backends

pando-rt can be linked against one backend at a time that each offers unique functionality. The backends that are currently supported are:
* [PANDO Runtime Exercise Platform](docs/PREP.md) (PREP), a Linux-based emulator to facilitate application development,
* [Drv-x](docs/DRVX.md), an [SST](https://sst-simulator.org/)-based simulator that compiles applications to native instructions, and
* [Drv-r](https://github.com/AMDResearch/pando-drv) (work-in-progress), an [SST](https://sst-simulator.org/)-based simulator that compiles applications to PandoHammer (RISC-V) instructions.

## Other Resources

* If you are a developer and want to contribute, make sure you read [this document](docs/developer.md).
* For an overview of the memory allocation support, please read [this document](docs/memory-resources.md).
* For an overview of the PGAS supported by PANDO, please read [this document](docs/PGAS.md).

## Quick Compilation and Execution Example

To compile pando-rt with PREP and run the `helloworld` example, you need to do the following:

```bash
# install spack and the dependent packages
git clone https://github.com/spack/spack.git ~/spack
. ~/spack/share/spack/setup-env.sh
spack install cmake gasnet qthreads
spack load cmake gasnet qthreads

# configure/build
mkdir -p build
cd build
cmake .. -DPANDO_RT_BACKEND=PREP
cmake --build . --parallel 8

# run
../scripts/preprun.sh -c 2 -n 2 ./examples/helloworld
```

For more information, please visit the pages for [PREP](docs/PREP.md) and [Drv](https://github.com/AMDResearch/pando-drv).
