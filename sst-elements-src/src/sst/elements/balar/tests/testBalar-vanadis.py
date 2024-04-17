# Automatically generated SST Python input
# Run script: sst testBalar-vanadis.py --model-options='-c gpu-v100-mem.cfg'

# This script will run the vanadisHandshake binary that perform a
# vectoradd progam similar to a native CUDA program launch with
# vanadis as the host CPU and balar+GPGPUSim as the CUDA device

import os
import sst
try:
    import ConfigParser
except ImportError:
    import configparser as ConfigParser
import argparse
from utils import *
# from mhlib import componentlist

# Arguments
parser = argparse.ArgumentParser()
parser.add_argument(
    "-c", "--config", help="specify configuration file", required=True)
parser.add_argument("-v", "--verbose",
                    help="increase verbosity of output", action="store_true")
parser.add_argument("-s", "--statfile",
                    help="statistics file", default="./stats.out")
parser.add_argument("-l", "--statlevel",
                    help="statistics level", type=int, default=16)
parser.add_argument(
    "-x", "--binary", help="specify input cuda binary", default="")
parser.add_argument("-a", "--arguments",
                    help="colon sep binary arguments", default="")


args = parser.parse_args()

verbose = args.verbose
cfgFile = args.config
statFile = args.statfile
statLevel = args.statlevel
binaryFile = args.binary
binaryArgs = args.arguments

# Build Configuration Information
config = Config(cfgFile, verbose=verbose)

DEBUG_L1 = 0
DEBUG_MEM = 0
DEBUG_CORE = 0
DEBUG_NIC = 0
DEBUG_LEVEL = 10

debug_params = {"debug": 1, "debug_level": 10}

# On network: Core, L1, MMIO device, memory
# Logical communication: Core->L1->memory
#                        Core->MMIO
#                        MMIO->memory
core_group = 0
mmio_group = 1
dma_group = 2
cache_group = 3
memory_group = 4

core_dst = [cache_group, mmio_group]
cache_src = [core_group, mmio_group, dma_group]
cache_dst = [memory_group]
mmio_src = [core_group]
mmio_dst = [dma_group, cache_group]
dma_src = [mmio_group]
dma_dst = [cache_group]
memory_src = [cache_group]

# Constans shared across components
network_bw = "25GB/s"
clock = "2GHz"
mmio_addr = 0xFFFF1000
balar_mmio_size = 1024
dma_mmio_addr = mmio_addr + balar_mmio_size
dma_mmio_size = 512

mmio = sst.Component("balar", "balar.balarMMIO")
mmio.addParams({
    "verbose": 20,
    "clock": clock,
    "base_addr": mmio_addr,
    "mmio_size": balar_mmio_size,
    "dma_addr": dma_mmio_addr,
})
mmio.addParams(config.getGPUConfig())

mmio_iface = mmio.setSubComponent("iface", "memHierarchy.standardInterface")
# mmio_iface.addParams(debug_params)

mmio_nic = mmio_iface.setSubComponent("memlink", "memHierarchy.MemNIC")
mmio_nic.addParams({"group": mmio_group,
                    "network_bw": network_bw,
                    "sources": mmio_src,
                    "destinations": mmio_dst,
                    })

# DMA Engine
dma = sst.Component("dmaEngine", "balar.dmaEngine")
dma.addParams({
    "verbose": 20,
    "clock": clock,
    "mmio_addr": dma_mmio_addr,
    "mmio_size": dma_mmio_size,
})
dma_if = dma.setSubComponent("iface", "memHierarchy.standardInterface")
dma_nic = dma_if.setSubComponent("memlink", "memHierarchy.MemNIC")
dma_nic.addParams({"group": dma_group,
                   "network_bw": network_bw,
                   "sources": dma_src,
                   "destinations": dma_dst
                   })

# GPU Memory hierarchy
#          mmio/GPU
#           |
#           L1
#           |
#           L2
#           |
#           mem port
#
# GPU Memory hierarchy configuration
print("Configuring GPU Network-on-Chip...")

# GPU Xbar group
l1g_group = 1
l2g_group = 2

gpu_router_ports = config.gpu_cores + config.gpu_l2_parts

GPUrouter = sst.Component("gpu_xbar", "shogun.ShogunXBar")
GPUrouter.addParams(config.getGPUXBarParams())
GPUrouter.addParams({
    "verbose": 1,
    "port_count": gpu_router_ports,
})

# Connect Cores & l1caches to the router
for next_core_id in range(config.gpu_cores):
    print("Configuring GPU core %d..." % next_core_id)

    gpuPort = "requestGPUCacheLink%d" % next_core_id

    l1g = sst.Component("l1gcache_%d" % (next_core_id), "memHierarchy.Cache")
    l1g.addParams(config.getGPUL1Params())
    l1g.addParams(debug_params)

    l1g_gpulink = l1g.setSubComponent("cpulink", "memHierarchy.MemLink")
    l1g_memlink = l1g.setSubComponent("memlink", "memHierarchy.MemNIC")
    l1g_memlink.addParams({"group": l1g_group})
    l1g_linkctrl = l1g_memlink.setSubComponent(
        "linkcontrol", "shogun.ShogunNIC")

    connect("gpu_cache_link_%d" % next_core_id,
            mmio, gpuPort,
            l1g_gpulink, "port",
            config.default_link_latency).setNoCut()

    connect("l1gcache_%d_link" % next_core_id,
            l1g_linkctrl, "port",
            GPUrouter, "port%d" % next_core_id,
            config.default_link_latency)

# Connect GPU L2 caches to the routers
num_L2s_per_stack = config.gpu_l2_parts // config.hbmStacks
sub_mems = config.gpu_memory_controllers
total_mems = config.hbmStacks * sub_mems

interleaving = 256
next_mem = 0
cacheStartAddr = 0x00
memStartAddr = 0x00

if (config.gpu_l2_parts % total_mems) != 0:
    print("FAIL Number of L2s (%d) must be a multiple of the total number memory controllers (%d)." % (
        config.gpu_l2_parts, total_mems))
    raise SystemExit

for next_group_id in range(config.hbmStacks):
    next_cache = next_group_id * sub_mems

    mem_l2_bus = sst.Component(
        "mem_l2_bus_" + str(next_group_id), "memHierarchy.Bus")
    mem_l2_bus.addParams({
        "bus_frequency": "4GHz",
        "drain_bus": "1"
    })

    for sub_group_id in range(sub_mems):
        memStartAddr = 0 + (256 * next_mem)
        endAddr = memStartAddr + \
            config.gpu_memory_capacity_inB - (256 * total_mems)

        if backend == "simple":
            # Create SimpleMem
            print("Configuring Simple mem part" + str(next_mem) +
                  " out of " + str(config.hbmStacks) + "...")
            mem = sst.Component("Simplehbm_" + str(next_mem),
                                "memHierarchy.MemController")
            mem.addParams(config.get_GPU_mem_params(
                total_mems, memStartAddr, endAddr))
            membk = mem.setSubComponent("backend", "memHierarchy.simpleMem")
            membk.addParams(config.get_GPU_simple_mem_params())
        elif backend == "ddr":
            # Create DDR (Simple)
            print("Configuring DDR-Simple mem part" + str(next_mem) +
                  " out of " + str(config.hbmStacks) + "...")
            mem = sst.Component("DDR-shbm_" + str(next_mem),
                                "memHierarchy.MemController")
            mem.addParams(config.get_GPU_ddr_memctrl_params(
                total_mems, memStartAddr, endAddr))
            membk = mem.setSubComponent("backend", "memHierarchy.simpleDRAM")
            membk.addParams(config.get_GPU_simple_ddr_params(next_mem))
        elif backend == "timing":
            # Create DDR (Timing)
            print("Configuring DDR-Timing mem part" + str(next_mem) +
                  " out of " + str(config.hbmStacks) + "...")
            mem = sst.Component("DDR-thbm_" + str(next_mem),
                                "memHierarchy.MemController")
            mem.addParams(config.get_GPU_ddr_memctrl_params(
                total_mems, memStartAddr, endAddr))
            membk = mem.setSubComponent("backend", "memHierarchy.timingDRAM")
            membk.addParams(config.get_GPU_ddr_timing_params(next_mem))
        else:
            # Create CramSim HBM
            print("Creating HBM controller " + str(next_mem) +
                  " out of " + str(config.hbmStacks) + "...")

            mem = sst.Component("GPUhbm_" + str(next_mem),
                                "memHierarchy.MemController")
            mem.addParams(config.get_GPU_hbm_memctrl_cramsim_params(
                total_mems, memStartAddr, endAddr))

            cramsimBridge = sst.Component(
                "hbm_cs_bridge_" + str(next_mem), "CramSim.c_MemhBridge")
            cramsimCtrl = sst.Component(
                "hbm_cs_ctrl_" + str(next_mem), "CramSim.c_Controller")
            cramsimDimm = sst.Component(
                "hbm_cs_dimm_" + str(next_mem), "CramSim.c_Dimm")

            cramsimBridge.addParams(
                config.get_GPU_hbm_cramsim_bridge_params(next_mem))
            cramsimCtrl.addParams(
                config.get_GPU_hbm_cramsim_ctrl_params(next_mem))
            cramsimDimm.addParams(
                config.get_GPU_hbm_cramsim_dimm_params(next_mem))

            linkMemBridge = sst.Link("memctrl_cramsim_link_" + str(next_mem))
            linkMemBridge.connect((mem, "cube_link", "2ns"),
                                  (cramsimBridge, "cpuLink", "2ns"))
            linkBridgeCtrl = sst.Link("cramsim_bridge_link_" + str(next_mem))
            linkBridgeCtrl.connect(
                (cramsimBridge, "memLink", "1ns"), (cramsimCtrl, "txngenLink", "1ns"))
            linkDimmCtrl = sst.Link("cramsim_dimm_link_" + str(next_mem))
            linkDimmCtrl.connect(
                (cramsimCtrl, "memLink", "1ns"), (cramsimDimm, "ctrlLink", "1ns"))

        print(" - Capacity: " + str(config.gpu_memory_capacity_inB //
              config.hbmStacks) + " per HBM")
        print(" - Start Address: " + str(hex(memStartAddr)) +
              " End Address: " + str(hex(endAddr)))

        connect("bus_mem_link_%d" % next_mem,
                mem_l2_bus, "low_network_%d" % sub_group_id,
                mem, "direct_link",
                "50ps").setNoCut()

        next_mem = next_mem + 1

    for next_mem_id in range(num_L2s_per_stack):
        cacheStartAddr = 0 + (256 * next_cache)
        endAddr = cacheStartAddr + \
            config.gpu_memory_capacity_inB - (256 * total_mems)

        print("Creating L2 controller %d-%d (%d)..." %
              (next_group_id, next_mem_id, next_cache))
        print(" - Start Address: " + str(hex(cacheStartAddr)) +
              " End Address: " + str(hex(endAddr)))

        l2g = sst.Component("l2gcache_%d" % (next_cache), "memHierarchy.Cache")
        l2g.addParams(config.getGPUL2Params(cacheStartAddr, endAddr))
        l2g_gpulink = l2g.setSubComponent("cpulink", "memHierarchy.MemNIC")
        l2g_gpulink.addParams({"group": l2g_group})
        l2g_linkctrl = l2g_gpulink.setSubComponent(
            "linkcontrol", "shogun.ShogunNIC")
        l2g_memlink = l2g.setSubComponent("memlink", "memHierarchy.MemLink")

        connect("l2g_xbar_link_%d" % (next_cache),
                GPUrouter, "port%d" % (config.gpu_cores+(next_cache)),
                l2g_linkctrl, "port",
                config.default_link_latency).setNoCut()

        connect("l2g_mem_link_%d" % (next_cache),
                l2g_memlink, "port",
                mem_l2_bus, "high_network_%d" % (next_mem_id),
                config.default_link_latency).setNoCut()

        print("++ %d-%d (%d)..." %
              (next_cache, sub_mems, (next_cache + 1) % sub_mems))
        if (next_cache + 1) % sub_mems == 0:
            next_cache = next_cache + total_mems - (sub_mems - 1)
        else:
            next_cache = next_cache + 1


# ===========================================================
# Enable statistics
sst.setStatisticLoadLevel(statLevel)
sst.enableAllStatisticsForAllComponents({"type": "sst.AccumulatorStatistic"})
sst.setStatisticOutput("sst.statOutputTXT", {"filepath": statFile})

print("Completed configuring the cuda-test model")


# ===========================================================
# Begin configuring vanadis core
# ===========================================================

mh_debug_level = 20
mh_debug = 1
dbgAddr = 0
stopDbg = 0

pythonDebug = False

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "0 ns")

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)

# ISA selection
vanadis_isa = os.getenv("VANADIS_ISA", "MIPS")
isa = "mipsel"
# vanadis_isa = os.getenv("VANADIS_ISA", "RISCV64")
# isa="riscv64"

loader_mode = os.getenv("VANADIS_LOADER_MODE", "0")

physMemSize = "4GiB"

exe = "vanadisHandshake"

# TODO: Weili: Add RISCV binary, currently just use MIPS for testing
# full_exe_name = os.getenv("VANADIS_EXE", "./vanadisHandshake/" + isa + "/" + exe )
full_exe_name = os.getenv("VANADIS_EXE", "./vanadisHandshake/" + exe)
exe_name = full_exe_name.split("/")[-1]

verbosity = int(os.getenv("VANADIS_VERBOSE", 0))
os_verbosity = os.getenv("VANADIS_OS_VERBOSE", verbosity)
pipe_trace_file = os.getenv("VANADIS_PIPE_TRACE", "")
lsq_entries = os.getenv("VANADIS_LSQ_ENTRIES", 32)

rob_slots = os.getenv("VANADIS_ROB_SLOTS", 64)
retires_per_cycle = os.getenv("VANADIS_RETIRES_PER_CYCLE", 4)
issues_per_cycle = os.getenv("VANADIS_ISSUES_PER_CYCLE", 4)
decodes_per_cycle = os.getenv("VANADIS_DECODES_PER_CYCLE", 4)

integer_arith_cycles = int(os.getenv("VANADIS_INTEGER_ARITH_CYCLES", 2))
integer_arith_units = int(os.getenv("VANADIS_INTEGER_ARITH_UNITS", 2))
fp_arith_cycles = int(os.getenv("VANADIS_FP_ARITH_CYCLES", 8))
fp_arith_units = int(os.getenv("VANADIS_FP_ARITH_UNITS", 2))
branch_arith_cycles = int(os.getenv("VANADIS_BRANCH_ARITH_CYCLES", 2))

auto_clock_sys = os.getenv("VANADIS_AUTO_CLOCK_SYSCALLS", "no")

cpu_clock = os.getenv("VANADIS_CPU_CLOCK", "2.3GHz")

numCpus = int(os.getenv("VANADIS_NUM_CORES", 1))
numThreads = int(os.getenv("VANADIS_NUM_HW_THREADS", 1))

vanadis_cpu_type = "vanadis."
vanadis_cpu_type += os.getenv("VANADIS_CPU_ELEMENT_NAME", "dbg_VanadisCPU")

if (verbosity > 0):
    print("Verbosity: " + str(verbosity) +
          " -> loading Vanadis CPU type: " + vanadis_cpu_type)
    print("Auto-clock syscalls: " + str(auto_clock_sys))
# vanadis_cpu_type = "vanadisdbg.VanadisCPU"

# TODO Weili: Add support for passing in CUDA program as argument?
app_args = os.getenv("VANADIS_EXE_ARGS", "./vectorAdd/vectorAdd")

vanadis_decoder = "vanadis.Vanadis" + vanadis_isa + "Decoder"
vanadis_os_hdlr = "vanadis.Vanadis" + vanadis_isa + "OSHandler"

protocol = "MESI"

# OS related params
osParams = {
    "processDebugLevel": 10,
    "verbose": os_verbosity,
    "cores": numCpus,
    "hardwareThreadCount": numThreads,
    "heap_start": 512 * 1024 * 1024,
    "heap_end": (2 * 1024 * 1024 * 1024) - 4096,
    "page_size": 4096,
    "heap_verbose": verbosity,
    "physMemSize": physMemSize,
    "useMMU": False,
}


processList = (
    (1, {
        "env_count": 2,
        "env0": "HOME=/home/sdhammo",
        "env1": "NEWHOME=/home/sdhammo2",
        "exe": full_exe_name,
        "arg0": exe_name,
    }),
    # ( 1, {
    #    "env_count" : 2, "env0" : "HOME=/home/sdhammo", "env1" : "NEWHOME=/home/sdhammo2", "argc" : 1, "exe" : "./tests/small/basic-io/hello-world/mipsel/hello-world",
    # "exe" : "./tests/small/basic-io/read-write/mipsel/read-write",
    # } ),
)

osl1cacheParams = {
    "access_latency_cycles": "2",
    "cache_frequency": cpu_clock,
    "replacement_policy": "lru",
    "coherence_protocol": protocol,
    "associativity": "8",
    "cache_line_size": "64",
    "cache_size": "32 KB",
    "L1": "1",
    "debug": mh_debug,
    "debug_level": mh_debug_level,
}

cpuCacheBalarRtrParams = {
    "xbar_bw": "1GB/s",
    "id": "0",
    "input_buf_size": "1KB",
    "num_ports": "4",
    "flit_size": "72B",
    "output_buf_size": "1KB",
    "link_bw": "1GB/s",
    "topology": "merlin.singlerouter"
}

memRtrParams = {
    "xbar_bw": "1GB/s",
    "link_bw": "1GB/s",
    "input_buf_size": "2KB",
    "num_ports": str(numCpus+2),
    "flit_size": "72B",
    "output_buf_size": "2KB",
    "id": "1",
    "topology": "merlin.singlerouter"
}

dirCtrlParams = {
    "coherence_protocol": protocol,
    "entry_cache_size": "1024",
    "debug": mh_debug,
    "debug_level": mh_debug_level,
    "addr_range_start": "0x0",
    "addr_range_end": "0xFFFFFFFF"
}

dirNicParams = {
    "network_bw": "25GB/s",
    "group": memory_group,
}

memCtrlParams = {
    "clock": cpu_clock,
    "backend.mem_size": physMemSize,
    "backing": "malloc",
    "initBacking": 1,
    "addr_range_start": 0,
    "addr_range_end": 0xffffffff,
    "debug_level": mh_debug_level,
    "debug": mh_debug,

}

memParams = {
    "mem_size": "4GiB",
    "access_time": "1 ns"
}

decoderParams = {
    "loader_mode": loader_mode,
    "uop_cache_entries": 1536,
    "predecode_cache_entries": 4
}

osHdlrParams = {
    "verbose": os_verbosity,
}

branchPredParams = {
    "branch_entries": 32
}

cpuParams = {
    "clock": cpu_clock,
    "verbose": verbosity,
    "hardware_threads": numThreads,
    "physical_fp_registers": 168,
    "physical_int_registers": 180,
    "integer_arith_cycles": integer_arith_cycles,
    "integer_arith_units": integer_arith_units,
    "fp_arith_cycles": fp_arith_cycles,
    "fp_arith_units": fp_arith_units,
    "branch_unit_cycles": branch_arith_cycles,
    "print_int_reg": False,
    "print_fp_reg": False,
    "pipeline_trace_file": pipe_trace_file,
    "reorder_slots": rob_slots,
    "decodes_per_cycle": decodes_per_cycle,
    "issues_per_cycle":  issues_per_cycle,
    "retires_per_cycle": retires_per_cycle,
    "auto_clock_syscall": auto_clock_sys,
    "pause_when_retire_address": os.getenv("VANADIS_HALT_AT_ADDRESS", 0),
    "start_verbose_when_issue_address": dbgAddr,
    "stop_verbose_when_retire_address": stopDbg,
    "print_rob": False,
}

lsqParams = {
    "verbose": verbosity,
    "address_mask": 0xFFFFFFFF,
    "load_store_entries": lsq_entries,
    "fault_non_written_loads_after": 0,
    "check_memory_loads": "no"
}

l1dcacheParams = {
    "access_latency_cycles": "2",
    "cache_frequency": cpu_clock,
    "replacement_policy": "lru",
    "coherence_protocol": protocol,
    "associativity": "8",
    "cache_line_size": "64",
    "cache_size": "32 KB",
    "L1": "1",
    "debug": mh_debug,
    "debug_level": mh_debug_level,
    "addr_range_start": 0,
    "addr_range_end": mmio_addr - 1,
}

l1icacheParams = {
    "access_latency_cycles": "2",
    "cache_frequency": cpu_clock,
    "replacement_policy": "lru",
    "coherence_protocol": protocol,
    "associativity": "8",
    "cache_line_size": "64",
    "cache_size": "32 KB",
    "prefetcher": "cassini.NextBlockPrefetcher",
    "prefetcher.reach": 1,
    "L1": "1",
    "debug": mh_debug,
    "debug_level": mh_debug_level,
}

l2cacheParams = {
    "access_latency_cycles": "14",
    "cache_frequency": cpu_clock,
    "replacement_policy": "lru",
    "coherence_protocol": protocol,
    "associativity": "16",
    "cache_line_size": "64",
    "cache_size": "1MB",
    "mshr_latency_cycles": 3,
    "debug": mh_debug,
    "debug_level": mh_debug_level,
}
busParams = {
    "bus_frequency": cpu_clock,
}

l2memLinkParams = {
    "group": cache_group,
    "network_bw": "25GB/s"
}


class CPU_Builder:
    def __init__(self):
        pass

    # CPU
    def build(self, prefix, nodeId, cpuId):

        if pythonDebug:
            print("build {}".format(prefix))

        # CPU
        cpu = sst.Component(prefix, vanadis_cpu_type)
        cpu.addParams(cpuParams)
        cpu.addParam("core_id", cpuId)
        cpu.enableAllStatistics()

        # CPU Argument
        if app_args != "":
            app_args_list = app_args.split(" ")
            # We have a plus 1 because the executable name is arg0
            app_args_count = len(app_args_list) + 1
            cpu.addParams({"app.argc": app_args_count})
            if (verbosity > 0):
                print("Identified " + str(app_args_count) +
                      " application arguments, adding to input parameters.")
            arg_start = 1
            for next_arg in app_args_list:
                if (verbosity > 0):
                    print("arg" + str(arg_start) + " = " + next_arg)
                cpu.addParams({"app.arg" + str(arg_start): next_arg})
                arg_start = arg_start + 1
        else:
            if (verbosity > 0):
                print("No application arguments found, continuing with argc=0")

        # CPU.decoder
        for n in range(numThreads):
            decode = cpu.setSubComponent("decoder"+str(n), vanadis_decoder)
            decode.addParams(decoderParams)

            decode.enableAllStatistics()

            # CPU.decoder.osHandler
            os_hdlr = decode.setSubComponent("os_handler", vanadis_os_hdlr)
            os_hdlr.addParams(osHdlrParams)

            # CPU.decocer.branch_pred
            branch_pred = decode.setSubComponent(
                "branch_unit", "vanadis.VanadisBasicBranchUnit")
            branch_pred.addParams(branchPredParams)
            branch_pred.enableAllStatistics()

        # CPU.lsq
        cpu_lsq = cpu.setSubComponent(
            "lsq", "vanadis.VanadisBasicLoadStoreQueue")
        cpu_lsq.addParams(lsqParams)
        cpu_lsq.enableAllStatistics()

        # CPU.lsq mem interface which connects to D-cache
        cpuDcacheIf = cpu_lsq.setSubComponent(
            "memory_interface", "memHierarchy.standardInterface")

        # CPU Dcache NIC
        cpuDcache_nic = cpuDcacheIf.setSubComponent(
            "memlink", "memHierarchy.MemNIC")
        cpuDcache_nic.addParams({
            "group": core_group,
            "network_bw": "25GB/s",
            # Allow cpu dcache to access cache and balarMMIO
            "destinations": core_dst})
        # end nic

        # CPU.mem interface for I-cache
        cpuIcacheIf = cpu.setSubComponent(
            "mem_interface_inst", "memHierarchy.standardInterface")

        # L1 D-cache
        cpu_l1dcache = sst.Component(
            prefix + ".l1dcache", "memHierarchy.Cache")
        cpu_l1dcache.addParams(l1dcacheParams)

        # L1 D-cache to cpu via NIC
        l1dcache_nic = cpu_l1dcache.setSubComponent(
            "cpulink", "memHierarchy.MemNIC")
        l1dcache_nic.addParams({
            "group": cache_group,
            "network_bw": "25GB/s",
            "sources": cache_src,
        })
        # L1 D-cache to L2 interface
        l1dcache_2_l2cache = cpu_l1dcache.setSubComponent(
            "memlink", "memHierarchy.MemLink")

        # L1 I-cache
        cpu_l1icache = sst.Component(
            prefix + ".l1icache", "memHierarchy.Cache")
        cpu_l1icache.addParams(l1icacheParams)

        # CPU, L1D, MMIO Router
        chiprtr = sst.Component(prefix + ".chiprtr_nic", "merlin.hr_router")
        chiprtr.addParams(cpuCacheBalarRtrParams)
        chiprtr.setSubComponent("topology", "merlin.singlerouter")

        # L1 D-cache <--> CPU <--> balarMMIO
        link_cpu_rtr = sst.Link(prefix+".link_cpu_router")
        link_cpu_rtr.connect((cpuDcache_nic, "port", "1ns"),
                             (chiprtr, "port0", "1ns"))

        link_l1dcache_rtr = sst.Link(prefix+".link_cpu_l1d")
        link_l1dcache_rtr.connect(
            (l1dcache_nic, "port", '1ns'), (chiprtr, "port1", "1ns"))

        # Connect balarMMIO to chiprtr?
        link_balarmmio_rtr = sst.Link(prefix+".link_balarmmio")
        link_balarmmio_rtr.connect(
            (mmio_nic, "port", "1ns"), (chiprtr, "port2", "1ns"))

        # Connect DMA (move data between SST simulated memory space (vanadis) and simulator memory space) to chiprtr
        link_dma_rtr = sst.Link(prefix+".link_dma")
        link_dma_rtr.connect((dma_nic, "port", "1ns"),
                             (chiprtr, "port3", "1ns"))

        # L1 I-cache to cpu interface
        l1icache_2_cpu = cpu_l1icache.setSubComponent(
            "cpulink", "memHierarchy.MemLink")
        # L1 I-cache to L2 interface
        l1icache_2_l2cache = cpu_l1icache.setSubComponent(
            "memlink", "memHierarchy.MemLink")

        # L2 cache
        cpu_l2cache = sst.Component(prefix+".l2cache", "memHierarchy.Cache")
        cpu_l2cache.addParams(l2cacheParams)

        # L2 cache cpu interface
        l2cache_2_l1caches = cpu_l2cache.setSubComponent(
            "cpulink", "memHierarchy.MemLink")

        # L2 cache mem interface
        l2cache_2_mem = cpu_l2cache.setSubComponent(
            "memlink", "memHierarchy.MemNIC")
        l2cache_2_mem.addParams(l2memLinkParams)

        # L1 to L2 bus
        cache_bus = sst.Component(prefix+".bus", "memHierarchy.Bus")
        cache_bus.addParams(busParams)

        # data L1 -> bus
        link_l1dcache_l2cache_link = sst.Link(
            prefix+".link_l1dcache_l2cache_link")
        link_l1dcache_l2cache_link.connect(
            (l1dcache_2_l2cache, "port", "1ns"), (cache_bus, "high_network_0", "1ns"))

        # CPU -> instructional L1
        link_cpu_l1icache_link = sst.Link(prefix+".link_cpu_l1icache_link")
        link_cpu_l1icache_link.connect(
            (cpuIcacheIf, "port", "1ns"), (l1icache_2_cpu, "port", "1ns"))

        # instruction L1 -> bus
        link_l1icache_l2cache_link = sst.Link(
            prefix+".link_l1icache_l2cache_link")
        link_l1icache_l2cache_link.connect(
            (l1icache_2_l2cache, "port", "1ns"), (cache_bus, "high_network_1", "1ns"))

        # BUS to L2 cache
        link_bus_l2cache_link = sst.Link(prefix+".link_bus_l2cache_link")
        link_bus_l2cache_link.connect(
            (cache_bus, "low_network_0", "1ns"), (l2cache_2_l1caches, "port", "1ns"))

        return (cpu, "os_link", "5ns"), (l2cache_2_mem, "port", "1ns")


def addParamsPrefix(prefix, params):
    # print( prefix )
    ret = {}
    for key, value in params.items():
        # print( key, value )
        ret[prefix + "." + key] = value

    # print( ret )
    return ret


# node OS
node_os = sst.Component("os", "vanadis.VanadisNodeOS")
node_os.addParams(osParams)

num = 0
for i, process in processList:
    # print( process )
    for y in range(i):
        # print( "process", num )
        node_os.addParams(addParamsPrefix("process" + str(num), process))
        num += 1

if pythonDebug:
    print('total hardware threads ' + str(num))

# node OS memory interface to L1 data cache
node_os_mem_if = node_os.setSubComponent(
    "mem_interface", "memHierarchy.standardInterface")

# node OS l1 data cache
os_cache = sst.Component("node_os.cache", "memHierarchy.Cache")
os_cache.addParams(osl1cacheParams)
os_cache_2_cpu = os_cache.setSubComponent("cpulink", "memHierarchy.MemLink")
os_cache_2_mem = os_cache.setSubComponent("memlink", "memHierarchy.MemNIC")
os_cache_2_mem.addParams(l2memLinkParams)

# node memory router
comp_chiprtr = sst.Component("chiprtr", "merlin.hr_router")
comp_chiprtr.addParams(memRtrParams)
comp_chiprtr.setSubComponent("topology", "merlin.singlerouter")

# node directory controller
dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
dirctrl.addParams(dirCtrlParams)

# node directory controller port to memory
dirtoM = dirctrl.setSubComponent("memlink", "memHierarchy.MemLink")
# node directory controller port to cpu
dirNIC = dirctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
dirNIC.addParams(dirNicParams)

# node memory controller
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams(memCtrlParams)

# node memory controller port to directory controller
memToDir = memctrl.setSubComponent("cpulink", "memHierarchy.MemLink")

# node memory controller backend
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams(memParams)

# Directory controller to memory router
link_dir_2_rtr = sst.Link("link_dir_2_rtr")
link_dir_2_rtr.connect(
    (comp_chiprtr, "port"+str(numCpus), "1ns"), (dirNIC, "port", "1ns"))
link_dir_2_rtr.setNoCut()

# Directory controller to memory controller
link_dir_2_mem = sst.Link("link_dir_2_mem")
link_dir_2_mem.connect((dirtoM, "port", "1ns"), (memToDir, "port", "1ns"))
link_dir_2_mem.setNoCut()

# os -> os l1 cache
link_os_cache_link = sst.Link("link_os_cache_link")
link_os_cache_link.connect(
    (node_os_mem_if, "port", "1ns"), (os_cache_2_cpu, "port", "1ns"))
link_os_cache_link.setNoCut()

os_cache_2_rtr = sst.Link("os_cache_2_rtr")
os_cache_2_rtr.connect((os_cache_2_mem, "port", "1ns"),
                       (comp_chiprtr, "port"+str(numCpus+1), "1ns"))
os_cache_2_rtr.setNoCut()

cpuBuilder = CPU_Builder()

# build all CPUs
nodeId = 0
for cpu in range(numCpus):

    prefix = "node" + str(nodeId) + ".cpu" + str(cpu)
    os_hdlr, l2cache = cpuBuilder.build(prefix, nodeId, cpu)

    # CPU os handler -> node OS
    link_core_os_link = sst.Link(prefix + ".link_core_os_link")
    link_core_os_link.connect(os_hdlr, (node_os, "core" + str(cpu), "5ns"))

    # connect cpu L2 to router
    link_l2cache_2_rtr = sst.Link(prefix + ".link_l2cache_2_rtr")
    link_l2cache_2_rtr.connect(
        l2cache, (comp_chiprtr, "port" + str(cpu), "1ns"))
