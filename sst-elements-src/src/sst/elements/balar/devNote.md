# Balar devnote

Some notes on balar development

## Usage

After successful compilation and installation of SST core and SST elements, run:

```bash
cd tests/
# Run with testCPU
sst testBalar-testcpu.py --model-options='-c ariel-gpu-v100.cfg -v -x vectorAdd/vectorAdd'
# Run with vanadis core
sst testBalar-vanadis.py --model-options='-c ariel-gpu-v100.cfg'
```

Can also set `PTX_SIM_DEBUG=3` to enable debug output in GPGPU-Sim, check its manual for more details.

## TODO

1. [x] Link the custom cuda lib with nvcc executable
2. [x] Create an interface in Balar using MMIO to accept arguments and send results
    1. [x] Test if can receive and send data properly with standardCPU
    1. Need to source the gpgpusim and cuda script
        1. `source ../env-setup/sst_env_setup.sh`
        1. `source ../sst-gpgpusim-external/setup_environment`
    1. Need to make and install the sst
        1. `make -j`
        1. `make install`
    1. Run script: `sst testBalar-simple.py --model-options="-c ariel-gpu-v100.cfg -v" > tmp.out 2>&1`
3. [ ] Real test
    1. [x] How to generate test stimulus?
    2. [x] Actually pass arguments to gpgpusim
    3. [x] VectorAdd example?
    4. [x] Get cuda calls arguments from old balar
        1. [ ] Automated dump cuda call sequences and generate launch/config file to the testcpu to consume
        2. [x] Or NVBit CUDA calls?
4. [x] Like a unit-test feel use StandardCPU to pass stimulus (CUDA arguments) to balar
5. [ ] Put the packet at mmio address + 4 location and pass the address to the mmio address
    1. Also the packet response should be put at the same address as well

## Issues

1. [x] Need to wait for kernel launch to be completed before issuing D2H copies?
    1. [ ] Need to understand the stream operation
    1. [ ] Never execute `stream_operation::do_operation` when a kernel exists ahead, should be blocking though?
    1. [ ] Probable cause in `void stream_manager::push( stream_operation op )` not waiting for stream 0 to be empty? Thus D2H copy is skipped
        1. Disable it will not change the result
    1. [ ] Right now the design is to fit a call in one memory handle
        1. However, we need to delay the response send to testcpu
        2. Send the response when the api call finishes in `tick` function
2. [x] Simulation stuck at `add.s32 %r8, %r6, %r7;` for vectorAdd
    1. Looks like it is waiting for the register to be loaded from global memory
    1. `ld.global.u32 %r6, [%rd8];` and `ld.global.u32 %r7, [%rd6];`
    1. Issue with memory unit loading? But params loading works?
        1. Maybe memcpyH2D is wrong? Use the SST version?
        2. Or need other config on memory subsystem?
    1. Config for SST GPGPUSIM is different
        1. Mem subsystem is turned off
    1. Didnot implement `SST_pop_mem_reply`, which is used to push mem requests from SST cache to GPGPUSim
        1. `shader.cc:4121`
        1. Need to construct the memory subsystem for balarMMIO then
        1. Check `icnt_inject_request_packet_to_SST()`
            1. Send read/write request from GPU to SST
            1. SST(balar) handles the cache request and timing
            1. Then it send back data to GPU using the `SST_receive_mem_reply()`, which push the memory packet to icnt for the GPU to access
                1. i.e. the load and store data goes here
    1. [x] Response from cache link does not match the id?
        1. Probably due to the way links are configured?
        1. Cache request is not converted to response? When using standardmem
        1. Cache never hit sending event to lower level, but only high level
            1. Cache request should send to lower level
            2. but instead send back to higher level (balar)
            3. in `CoherenceController::forwardByAddress()`
            4. `linkDown_` in `coherenceController.cc:CoherenceController::forwardByAddress()` could not find the target destination?
            5. Need to configure cache range?
            6. NVM, copied the old balar python launch script, which did not configure the memory range properly, causing cuda mem access to be out of range and thus redirected to higher layer in mem system (balar in this case).
3. [ ] Stat matching with original balar
    1. Most differences due to simulation time
    1. Latency for l1gcache and l2gcache looks fairly close
    1. Filter out this cause, we have
        1. L1gcache/L2gcache
            1. Due to `GetX_recv`, `GetXResp_recv` count?
        2. SimpleHbm
            1. Outstanding request
            1. Cycles with issue
            1. Total cycles

## Note

1. Use `--output-dot [filename]` to output a dot file describing the graph and use `dot -Tpng [File.dot] > [File.png]` to output to png format
1. Build configuration
    1. With PIN: `source sst_env_setup_10_1.sh`
        1. CUDA 10.1, gcc 4.9.2 for PIN
    2. Without Pin: `source 10.1_env_setup.sh`
        1. CUDA 10.1, gcc 7.5.0
    3. sst-core: `./configure --prefix=$SST_CORE_HOME --disable-mpi --disable-mem-pools`
    4. Source setup for CUDA: `source SST_EXTERNAL_PATH/setup_environment`
        1. Also need to build the gpgpusim beforehand
    5. sst-elements: `./configure --prefix=$SST_ELEMENTS_HOME --with-sst-core=$SST_CORE_HOME --with-pin=$PIN_HOME --with-cuda=$CUDA_INSTALL_PATH --with-gpgpusim=$GPGPUSIM_ROOT`
    6. Without PIN (mmio balar only): `./configure --prefix=$SST_ELEMENTS_HOME --with-sst-core=$SST_CORE_HOME --with-cuda=$CUDA_INSTALL_PATH --with-gpgpusim=$GPGPUSIM_ROOT`
1. Can build sst-core with `--enable-debug` to enable memHierarchy debug output, useful for debugging
1. Filter memory subsystem stats from SST run
    1. `grep -E -e '(l1gcache_.*)|(l2gcache_.*)|(Simplehbm_.*)' stats.out > mmio_mem.out`
    1. `grep -E -e '(l1gcache_.*)|(l2gcache_.*)|(Simplehbm_.*)' refFiles/test_gpgpu_vectorAdd.out > prev_mem.out`
    1. Compare with `git diff --word-diff=color --no-index mmio_mem.out prev_mem.out`
1. Try vanadis handshake binary: `sst testBalar-vanadis.py --model-options='-c ariel-gpu-v100.cfg'`
