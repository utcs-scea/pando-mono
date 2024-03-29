# Drv-X

## Introduction

[Drv-x](https://github.com/AMDResearch/pando-drv) is an [SST](https://sst-simulator.org/)-based simulator that compiles applications to native instructionsand simulates the PANDO hardware in a cycle-accurate manner. 

## Installing Drv-X

```bash
git clone git@github.com:AMDResearch/pando-drv.git --branch pando-rt-backend --depth 1 --single-branch
pushd pando-drv
. load_drvx.sh
./build_drvx_deps.sh
make install
popd
```

## Compilation

To compile pando-rt with Drv-X, you need to do the following:

1. Create a `build` directory:
```bash
mkdir -p build
```

2. Configure the project:
```bash
cd build
cmake -DDRVX_ROOT=$DRV_ROOT -DPANDO_RT_BACKEND=DRVX ..
```

3. Compile:
```bash
cmake --build .
```

## Execution

To run an application, e.g., the `examples/helloworld`, with 1 node, 2 cores and 4 SST threads, you may use the following command:
```bash
../scripts/drvxrun.sh -c 2 -n 1 -s 4 examples/libhelloworld.so
```
or without `drvxrun.sh`:
```bash
sst -n 4 $DRV_ROOT/../tests/PANDOHammerDrvX.py -- --with-command-processor=examples/libhelloworld.so --num-pxn=1 --pod-cores=2 --drvx-stack-in-l1sp examples/libhelloworld.so
```
