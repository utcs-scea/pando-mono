# PANDO Runtime Exercise Platform

## Introduction

PANDO Runtime Exercise Platform (PREP) is a hardware emulator of the PANDO system, written in software. It emulates all aspects of the PANDO hardware as such that they resemble the hardware as close as possible even if it comes at a performance cost. PREP is compiled to the host's native instruction set.

PREP is developed to:
1. Simplify software development by offering a PANDO system behavior over traditional platforms, such as multi-node configurations of x86 processor nodes.
2. Facilitate co-design by allow rapid iteration of adding new emulated hardware capabilities and using them in software.
3. Serve as a testing vehicle for new PANDO APIs without relying on introducing new hardware.
4. Identify PANDO hardware components that need to be exposed via APIs.
5. Assist in porting PANDO software to PANDO hardware.
6. Offer a convenient environment to conduct what-if experiments and identify potential bottlenecks.

PREP cannot be used by itself to infer about algorithm or data structure performance. While useful metrics can be extracted from it, e.g., communication patterns, message sizes, relative work scheduling, wall-clock time is not expected to reflect behavior of the PANDO hardware. As such, optimizing software for PREP may not result in the same results on real PANDO hardware.

## Prerequisites

PREP requires:
* pando-rt [prerequisites](../README.md#prerequisites)
* [GASNet](https://gasnet.lbl.gov/)
* [Qthreads](https://www.sandia.gov/qthreads/)

It can optionally use:
* An MPI implementation to compile the MPI GASNet conduit

The easiest way to install all of these is via [spack](https://spack.io/).
```bash
git clone https://github.com/spack/spack.git
. spack/share/spack/setup-env.sh
spack install cmake qthreads openmpi gasnet
spack load cmake qthreads openmpi gasnet
```

## Compilation

To compile pando-rt with PREP, you need to do the following:

1. Create a `build` directory:
```bash
mkdir build
```

2. Configure the project.

E.g., for using GASNet with the [`smp-conduit`](https://gasnet.lbl.gov/dist-ex/smp-conduit/README):
```bash
cd build
cmake .. -DPANDO_RT_BACKEND=PREP -DPANDO_PREP_GASNET_CONDUIT=smp
```

E.g., for using GASNet with the [`mpi-conduit`](https://gasnet.lbl.gov/dist-ex/mpi-conduit/README):
```bash
cd build
cmake .. -DPANDO_RT_BACKEND=PREP -DPANDO_PREP_GASNET_CONDUIT=mpi
```

Other GASNet conduits may work, but they have not been tested. If you omit `PANDO_PREP_GASNET_CONDUIT`, `smp` is implied.


3. Compile:
```bash
cd build
cmake --build .
```

## Execution

To run an application when using the `smp-conduit`, e.g., the `examples/helloworld`, with 2 nodes, 2 cores and 16 harts, you may use the following command:
```bash
./scripts/preprun.sh -c 2 -n 2 ./examples/helloworld
```
or without `preprun.sh`:
```bash
PANDO_PREP_NUM_CORES=2 GASNET_PSHM_NODES=2 ./examples/helloworld
```

When using the `mpi-conduit`, Address Space Layout Randomization (ASLR) needs to be turned off:
```bash
echo 0 | sudo tee -a /proc/sys/kernel/randomize_va_space
```
To run the same application with 2 nodes, 2 cores and 16 harts, you may use the following command:
```bash
./scripts/preprun_mpi.sh -c 2 -n 2 ./examples/helloworld
```
or without `preprun_mpi.sh`:
```bash
PANDO_PREP_NUM_CORES=2 gasnetrun_mpi -n 2 ./examples/helloworld
```

## Configuration

The following environment variables are supported

| Variable | Description | Default Value |
| --- | --- | --- |
| PANDO_PREP_LOG_LEVEL | String value (`error`, `warning`, `info`) that controls the level of logging. | `error`
| PANDO_PREP_NUM_CORES | Number of emulated PandoHammer cores per pod. | `8`
| PANDO_PREP_NUM_HARTS | Number of emulated PandoHammer harts (FGMT) per code. | `16`
| PANDO_PREP_L1SP_HART | Hart stack size in bytes. `PANDO_PREP_NUM_HARTS` * `PANDO_PREP_L1SP_HART` is the size of the L1SP per core. | `8192` (8KiB)
| PANDO_PREP_L2SP_POD | Per pod L2 scratchpad size in bytes. | `33554432` (32MiB)
| PANDO_PREP_MAIN_NODE | Per node main memory size in bytes. | `4294967296` (4GiB)
| PANDO_TRACING_LOG_PAYLOAD | String value (`off`, `on`) that controls logging payload when memory tracing is enabled. | `on`
| PANDO_TRACING_MEM_STAT_FILE_PREFIX | Memory stat file prefix. | empty string
| PANDO_PREP_GASNET_CONDUIT | String value (`smp`, `mpi`) that sets the conduit to use for GASNet. | `smp`

## Using Docker

If you'd like to use docker instead you can setup docker using the following command:
```bash
BUILD_TYPE=Release ./docker/docker_setup.sh
```
This creates a `dockerbuild` directory within the directory root, if you'd like to build you can run
```bash
./docker/docker_build.sh
```

Note that all interactions with this build should happen through the docker container.
To enter the container and mount your vim configuration run:
```bash
./docker/docker_run.sh
```
This will place you in the source directory.

## Debugging

### gdb

The host's gdb is supported, as PREP compiles to the native host's instruction set. No additional configuration is needed.

### Address Sanitizer (ASan)

If you are using [Address Sanitizer](https://github.com/google/sanitizers/wiki/AddressSanitizer) the stack space may not be enough due to increased memory pressure from ASan. Typical behavior of a filled up stack is seemingly random segfaults, e.g., when calling a function or returning from one.

You can increase the per-hart L1SP using the `PANDO_PREP_L1SP_HART` environment variable, e.g., if for 16KiB you can use
```bash
export PANDO_PREP_L1SP_HART=16384
```

You may also need to turn-off automatic test discovery by setting the CMake variable `PANDO_TEST_DISCOVERY=OFF` (see the [developer document](developer.md)).

----

## PandoHammer Emulation

PandoHammer chiplets are emulated using [qthreads](https://www.sandia.gov/qthreads/). A PandoHammer core is emulated with a qthreads shepherd (OS thread) which is assigned a fixed number of qthreads (coroutines) that each run code as an FGMT.

qthreads was chosen since it offers:
1. Hart (Fine-Grain Multithreading - FGMT) support via spawning qthreads on pinned shepherds.
2. Synchronization via full/empty bit support on individual words.
3. Task migration via qthread migration.
4. Configurable stacks and stack guards.
5. Queue and dictionary data structures.

Using qthreads allows us the flexibility to experiment with different stack sizes, number of harts, and the presence or not of thread migration.

### PandoHammer Core

A PandoHammer core is emulated via `PANDO_PREP_NUM_HARTS` qthreads associated with 1 shepherd. The qthreads are round-robin scheduled and the scheduling points are when the qthread make a PANDO-HAL API call. Since qthreads support cooperative multithreading, they can only willing be context switched as opposed to PandoHammer harts. As such, many pando-rt functions are context switching implicitly.

The L1SP is emulated via the stack that is provided by qthreads. Each qthread is associated with a stack that the size can be configured at runtime. The stack size is set to `PANDO_PREP_L1SP_HART` per qthread, for a total of (`PANDO_PREP_NUM_HARTS` * `PANDO_PREP_L1SP_HART`) for all the qthreads associated with a shepherd.

### PandoHammer Pod (work-in-progress)

A PANDOHammer pod is emulated by having 64 shepherds in a logical 2D arrangement and their associated qthreads form a logical group.

The L2SP is emulated via a contiguous memory chunk allocated at start.

### PXN (node)

A PXN is emulated via having a number of pods in a logical 2D arrangement.

The main memory is emulated via a contiguous memory chunk allocated at start.

----

## Communication

PREP uses the [GASNet-EX](https://gasnet.lbl.gov/) library for communication.

GASNet-EX was chosen since it offers:
1. Active messages support.
1. Collective operations support.
1. Subgroup support via GASNet teams.
1. RDMA non-blocking put and get support.

GASNet-EX is used as an abstraction of the interconnect of the PANDO system where each GASNet node is a PXN node. PREP uses the GASNet API as needed, offering only the primitives that are needed by the PANDO system.

----

## Remote Memory Accesses

The PANDO system has a least three (3) globally addressable, distributed memory (L1SP, L2SP, main memory). While in hardware and/or software emulation of a RISC-V it is possible to automatically determine which memory in which PXN a pointer points to, in PREP all such pointers need to be wrapped in a `GlobalPtr` object.

A `GlobalPtr` object behaves as a raw pointer and can be copied around. It has the necessarly functionality via proxy objects to do local or remote loads and stores. Additionally, each dereference will cause a context switch, which provides a more faithful emulation of the hardware context switching of FGMT.

Remote memory access, i.e., for now, remote loads and stores, are implemented using a pair GASNet-EX active messages, one for the request (`AMType::Load` and `AMType::Store`) and one for the reply (`AMType::LoadReply` and `AMType::StoreReply`). Both loads and stores are synchronizing, as is the case with CXL which is proposed for use in the PANDO system.

## Application Driver

PREP builds its own driver executable that kickstarts the PANDO runtime and invokes the user-defined `pandoMain` function. A special thread is spawned by PREP to emulate the command processor (CP). The CP thread and all PandoHammer cores/harts begin executing the same runtime code. The CP initializes the PANDO runtime and begins executing `pandoMain`. All PandoHammer cores/harts wait for work to be dispatched to their queues. Once `pandoMain` terminates, the CP cleans up the runtime and shuts down the PREP backend.
