# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington
import sst
import argparse
import re

# common functions
ADDR_TYPE_HI,ADDR_TYPE_LO     = (63, 58)
ADDR_PXN_HI, ADDR_PXN_LO      = (57, 44)
ADDR_POD_HI,ADDR_POD_LO       = (39, 34)
ADDR_CORE_Y_HI,ADDR_CORE_Y_LO = (30, 28)
ADDR_CORE_X_HI,ADDR_CORE_X_LO = (24, 22)

ADDR_TYPE_L1SP    = 0b000000
ADDR_TYPE_L2SP    = 0b000001
ADDR_TYPE_MAINMEM = 0b000100

def set_bits(word, hi, lo, value):
    mask = (1 << (hi - lo + 1)) - 1
    word &= ~(mask << lo)
    word |= (value & mask) << lo
    return word

# Function to convert frequency string to Hz
def freq_str_to_hz(frequency_str):
    # Regular expression to match frequency and its unit
    match = re.match(r"(\d+)([GMK]Hz)", frequency_str)
    if not match:
        raise ValueError("Invalid frequency format")

    value = int(match.group(1))
    unit = match.group(2)

    # Convert value to Hz based on unit
    if unit == "GHz":
        return value * 1e9
    elif unit == "MHz":
        return value * 1e6
    elif unit == "KHz":
        return value * 1e3
    else:
        raise ValueError("Unsupported frequency unit")

# Function to format bandwidth with appropriate units
def format_bw(bandwidth):
    if bandwidth >= 1e12:
        return f"{bandwidth / 1e12:.2f}TB/s"
    elif bandwidth >= 1e9:
        return f"{bandwidth / 1e9:.2f}GB/s"
    elif bandwidth >= 1e6:
        return f"{bandwidth / 1e6:.2f}MB/s"
    elif bandwidth >= 1e3:
        return f"{bandwidth / 1e3:.2f}KB/s"
    else:
        return f"{bandwidth:.2f}B/s"

################################
# parse command line arguments #
################################
parser = argparse.ArgumentParser()
parser.add_argument("program", help="program to run")
parser.add_argument("argv", nargs=argparse.REMAINDER, help="arguments to program")
parser.add_argument("--verbose", type=int, default=0, help="verbosity of core")
parser.add_argument("--dram-access-time", type=str, default="13ns", help="latency of DRAM (only valid if using the latency based model)")
parser.add_argument("--dram-backend", type=str, default="simple", choices=['simple', 'ramulator', 'dramsim3'], help="backend timing model for DRAM")
parser.add_argument("--dram-backend-config", type=str, default="/root/sst-ramulator-src/configs/hbm4-pando-config.cfg",
                    help="backend timing model configuration for DRAM")
parser.add_argument("--debug-init", action="store_true", help="enable debug of init")
parser.add_argument("--debug-memory", action="store_true", help="enable memory debug")
parser.add_argument("--debug-requests", action="store_true", help="enable debug of requests")
parser.add_argument("--debug-responses", action="store_true", help="enable debug of responses")
parser.add_argument("--debug-syscalls", action="store_true", help="enable debug of syscalls")
parser.add_argument("--debug-clock", action="store_true", help="enable debug of clock ticks")
parser.add_argument("--debug-mmio", action="store_true", help="enable debug of mmio requests to the core")
parser.add_argument("--verbose-memory", type=int, default=0, help="verbosity of memory")
parser.add_argument("--pod-cores", type=int, default=8, help="number of cores per pod")
parser.add_argument("--pxn-pods", type=int, default=1, help="number of pods")
parser.add_argument("--num-pxn", type=int, default=1, help="number of pxns")
parser.add_argument("--core-threads", type=int, default=16, help="number of threads per core")
parser.add_argument("--core-clock", type=str, default="1GHz", help="clock frequency of cores")
parser.add_argument("--core-max-idle", type=int, default=1, help="max idle time of cores")
parser.add_argument("--command-clock", type=str, default="2GHz", help="clock frequency of command processors")
parser.add_argument("--mem-clock", type=str, default="1GHz", help="clock frequency of memory components")

parser.add_argument("--pod-l2sp-banks", type=int, default=8, help="number of l2sp banks per pod")
parser.add_argument("--pod-l2sp-interleave", type=int, default=0, help="interleave size of l2sp addresses (defaults to no  interleaving)")

parser.add_argument("--pxn-dram-banks", type=int, default=8, help="number of dram banks per pxn")
parser.add_argument("--pxn-dram-size", type=int, default=1024**3, help="size of main memory per pxn (max {} bytes)".format(8*1024*1024*1024))
parser.add_argument("--pxn-dram-interleave", type=int, default=0, help="interleave size of dram addresses (defaults to no  interleaving)")

parser.add_argument("--network-issue-rate", type=int, default=8, help="core memory operation issue rate in bytes")
parser.add_argument("--network-bw-config", type=str, default="auto", choices=['auto', 'manual'], help="network bandwidth configuration type")
parser.add_argument("--network-onchip-bw", type=str, default="1GB/s", help="network on chip router bandwidth")
parser.add_argument("--network-offchip-bw", type=str, default="4GB/s", help="network off chip router bandwidth")
parser.add_argument("--network-onchip-buffer-size", type=str, default="1MB", help="on chip network input / output buffer size")
parser.add_argument("--network-offchip-buffer-size", type=str, default="64MB", help="off chip network input / output buffer size")

parser.add_argument("--with-command-processor", type=str, default="",
                    help="Command processor program to run. Defaults to empty string, in which no command processor will be included in the model.")
parser.add_argument("--cp-verbose", type=int, default=0, help="verbosity of command processor")
parser.add_argument("--cp-verbose-init", action="store_true", help="command processor enable debug of init")
parser.add_argument("--cp-verbose-requests", action="store_true", help="command processor enable debug of requests")
parser.add_argument("--cp-verbose-responses", action="store_true", help="command processor enable debug of responses")
parser.add_argument("--drvx-stack-in-l1sp", action="store_true", help="use l1sp backing storage as stack")
parser.add_argument("--drvr-isa-test", action="store_true", help="Running an ISA test")
parser.add_argument("--test-name", type=str, default="", help="Name of the test")
parser.add_argument("--core-stats", action="store_true", help="enable core statistics")
parser.add_argument("--all-stats", action="store_true", help="enable all statistics")
parser.add_argument("--perf-stats", action="store_true", help="enable performance statistics")
parser.add_argument("--stats-load-level", type=int, default=0, help="load level for statistics")
parser.add_argument("--stats-preallocated-phase", type=int, default=16, help="preallocated number of phases for statistics")
parser.add_argument("--trace-remote-pxn-memory", action="store_true", help="trace remote pxn memory accesses")

arguments = parser.parse_args()

#############################################
# Latencies of various components and links #
#############################################
# determine latency of link
LINK_LATENCIES = {
}
def link_latency(link_name):
    if link_name not in LINK_LATENCIES:
        return "1ps"

    return LINK_LATENCIES[link_name]

# determine the latency of a router
ROUTER_LATENCIES = {
    'tile_rtr': "250ps",
    'mem_rtr': "250ps",
    'chiprrtr': "250ps",
    'offchiprtr': "70ns",
}
def router_latency(router_name):
    if router_name not in ROUTER_LATENCIES:
        return "1ps"

    return ROUTER_LATENCIES[router_name]

# determine the latency of a memory
MEMORY_LATENCIES = {
    'l1sp': "1ns",
    'l2sp': "1ns",
    'dram': arguments.dram_access_time,
}
def memory_latency(memory_name):
    if memory_name not in MEMORY_LATENCIES:
        return "1ps"

    return MEMORY_LATENCIES[memory_name]

##################
# Size Constants #
##################
POD_L2SP_SIZE = (1<<25)
CORE_L1SP_SIZE = (1<<17)

###################
# router id bases #
###################
COMPUTETILE_RTR_ID = 0
SHAREDMEM_RTR_ID = 1024
CHIPRTR_ID = 1024*1024

###################
# Bandwidth Numbers #
###################
COMMAND_BW_NUM = arguments.network_issue_rate * freq_str_to_hz(arguments.command_clock)
COMMAND_BW = format_bw(COMMAND_BW_NUM)
CORE_BW_NUM = arguments.network_issue_rate * freq_str_to_hz(arguments.core_clock)
CORE_BW = format_bw(CORE_BW_NUM)
SCRATCHPAD_BW = format_bw(arguments.network_issue_rate * freq_str_to_hz(arguments.mem_clock))
L2_MEM_BANK_BW = format_bw(arguments.pod_cores * arguments.network_issue_rate * freq_str_to_hz(arguments.core_clock) / arguments.pod_l2sp_banks)
L2_MEM_BW_NUM = arguments.pod_cores * arguments.network_issue_rate * freq_str_to_hz(arguments.core_clock)
L2_MEM_BW = format_bw(L2_MEM_BW_NUM)
MAIN_MEM_BANK_BW = format_bw(arguments.pxn_pods * arguments.pod_cores * arguments.network_issue_rate * freq_str_to_hz(arguments.core_clock) / arguments.pxn_dram_banks)
MAIN_MEM_BW_NUM = arguments.pxn_pods * arguments.pod_cores * arguments.network_issue_rate * freq_str_to_hz(arguments.core_clock)
MAIN_MEM_BW = format_bw(MAIN_MEM_BW_NUM)
if arguments.network_bw_config == "manual":
    ONCHIP_RTR_BW = arguments.network_onchip_bw
    OFFCHIP_RTR_BW = arguments.network_offchip_bw
else:
    ONCHIP_RTR_BW_NUM = 2 * (COMMAND_BW_NUM + arguments.pxn_pods*arguments.pod_cores*CORE_BW_NUM + arguments.pxn_pods*L2_MEM_BW_NUM + MAIN_MEM_BW_NUM)
    ONCHIP_RTR_BW = format_bw(ONCHIP_RTR_BW_NUM)
    OFFCHIP_RTR_BW = format_bw(arguments.num_pxn * ONCHIP_RTR_BW_NUM)

SYSCONFIG = {
    "sys_num_pxn" : 1,
    "sys_pxn_pods" : 1,
    "sys_pod_cores" : 8,
    "sys_core_threads" : 16,
    "sys_core_clock" : "1GHz",
    "sys_core_l1sp_size" : CORE_L1SP_SIZE,
    "sys_pxn_dram_size" : arguments.pxn_dram_size,
    "sys_pxn_dram_ports" : arguments.pxn_dram_banks,
    "sys_pxn_dram_interleave_size" : arguments.pxn_dram_interleave if arguments.pxn_dram_interleave else arguments.pxn_dram_size//arguments.pxn_dram_banks,
    "sys_pod_l2sp_size" : POD_L2SP_SIZE,
    "sys_pod_l2sp_banks" : arguments.pod_l2sp_banks,
    "sys_pod_l2sp_interleave_size" : arguments.pod_l2sp_interleave if arguments.pod_l2sp_interleave else POD_L2SP_SIZE//arguments.pod_l2sp_banks,
    "sys_nw_flit_dwords" : 1,
    "sys_nw_obuf_dwords" : 8,
    "sys_cp_present" : False,
}

CORE_DEBUG = {
    "debug_init" : False,
    "debug_clock" : False,
    "debug_requests" : False,
    "debug_responses" : False,
    "debug_loopback" : False,
    "debug_mmio" : False,
    "debug_memory": False,
    "debug_syscalls" : False,
    "trace_remote_pxn" : False,
    "trace_remote_pxn_load" : False,
    "trace_remote_pxn_store" : False,
    "trace_remote_pxn_atomic" : False,
    "isa_test": False,
    "test_name": "",
}

KNOBS = {
    "core_max_idle" : arguments.core_max_idle,
}

SYSCONFIG['sys_num_pxn'] = arguments.num_pxn
SYSCONFIG['sys_pxn_pods'] = arguments.pxn_pods
SYSCONFIG['sys_pod_cores'] = arguments.pod_cores
SYSCONFIG['sys_core_threads'] = arguments.core_threads
SYSCONFIG['sys_cp_present'] = bool(arguments.with_command_processor)
SYSCONFIG['sys_core_clock'] = arguments.core_clock

CORE_DEBUG['debug_memory'] = arguments.debug_memory
CORE_DEBUG['debug_requests'] = arguments.debug_requests
CORE_DEBUG['debug_responses'] = arguments.debug_responses
CORE_DEBUG['debug_syscalls'] = arguments.debug_syscalls
CORE_DEBUG['debug_init'] = arguments.debug_init
CORE_DEBUG['debug_clock'] = arguments.debug_clock
CORE_DEBUG['debug_mmio'] = arguments.debug_mmio
CORE_DEBUG["trace_remote_pxn"] = arguments.trace_remote_pxn_memory
CORE_DEBUG['isa_test'] = arguments.drvr_isa_test
CORE_DEBUG['test_name'] = arguments.test_name

class L1SPRange(object):
    L1SP_SIZE = SYSCONFIG['sys_core_l1sp_size']
    L1SP_SIZE_STR = "128KiB"
    def __init__(self, pxn, pod, core_y, core_x):
        start = 0
        start = set_bits(start, ADDR_TYPE_HI, ADDR_TYPE_LO, ADDR_TYPE_L1SP)
        start = set_bits(start, ADDR_PXN_HI, ADDR_PXN_LO, pxn)
        start = set_bits(start, ADDR_POD_HI, ADDR_POD_LO, pod)
        start = set_bits(start, ADDR_CORE_Y_HI, ADDR_CORE_Y_LO, core_y)
        start = set_bits(start, ADDR_CORE_X_HI, ADDR_CORE_X_LO, core_x)
        self.start = start
        self.end = start + self.L1SP_SIZE - 1

class L2SPRange(object):
    # 16MiB
    L2SP_POD_BANKS = SYSCONFIG['sys_pod_l2sp_banks']
    L2SP_SIZE = SYSCONFIG['sys_pod_l2sp_size']
    L2SP_BANK_SIZE = L2SP_SIZE // L2SP_POD_BANKS
    L2SP_SIZE_STR = "16MiB"
    L2SP_BANK_SIZE_STR = "4MiB"
    # interleave
    L2SP_INTERLEAVE_SIZE = SYSCONFIG['sys_pod_l2sp_interleave_size']
    L2SP_INTERLEAVE_SIZE_STR = "{}B".format(L2SP_INTERLEAVE_SIZE)
    L2SP_INTERLEAVE_STEP = L2SP_INTERLEAVE_SIZE * L2SP_POD_BANKS
    L2SP_INTERLEAVE_STEP_STR = "{}B".format(L2SP_INTERLEAVE_STEP)

    def __init__(self, pxn, pod, bank):
        start = 0
        start = set_bits(start, ADDR_TYPE_HI, ADDR_TYPE_LO, ADDR_TYPE_L2SP)
        start = set_bits(start, ADDR_PXN_HI, ADDR_PXN_LO, pxn)
        start = set_bits(start, ADDR_POD_HI, ADDR_POD_LO, pod)
        self.interleave_size = self.L2SP_INTERLEAVE_SIZE
        self.interleave_step = self.L2SP_INTERLEAVE_STEP
        self.start = start \
            + bank * self.interleave_size
        self.end = start \
            + self.L2SP_SIZE \
            - (self.L2SP_POD_BANKS - bank - 1) * self.interleave_size \
            - 1

def MakeMainMemoryRangeClass(banks, size):
    if (size > 8*1024*1024*1024):
        raise ValueError("PXN main memory size cannot be more than 8GiB")

    class MainMemoryRange(object):
        # specs say upto 8TB
        # we make this a parameter here
        MAINMEM_BANKS = banks
        MAINMEM_SIZE = size
        MAINMEM_BANK_SIZE = MAINMEM_SIZE // MAINMEM_BANKS
        # size strings
        MAINMEM_SIZE_STR = "{}GiB".format(MAINMEM_SIZE // 1024**3)
        MAINMEM_BANK_SIZE_STR = "{}MiB".format(MAINMEM_BANK_SIZE // 1024**2)
        # interleave
        MAINMEM_INTERLEAVE_SIZE = SYSCONFIG['sys_pxn_dram_interleave_size']
        MAINMEM_INTERLEAVE_SIZE_STR = "{}B".format(MAINMEM_INTERLEAVE_SIZE)
        MAINMEM_INTERLEAVE_STEP = MAINMEM_INTERLEAVE_SIZE * MAINMEM_BANKS
        MAINMEM_INTERLEAVE_STEP_STR = "{}B".format(MAINMEM_INTERLEAVE_STEP)

        def __init__(self, pxn, pod, bank):
            start = 0
            start = set_bits(start, ADDR_TYPE_HI, ADDR_TYPE_LO, ADDR_TYPE_MAINMEM)
            start = set_bits(start, ADDR_PXN_HI, ADDR_PXN_LO, pxn)
            self.interleave_size = self.MAINMEM_INTERLEAVE_SIZE
            self.interleave_step = self.MAINMEM_INTERLEAVE_STEP
            self.start = start \
                + pod  * size  \
                + bank * self.interleave_size
            self.end = start \
                + pod * size \
                + self.MAINMEM_SIZE \
                - (self.MAINMEM_BANKS - bank - 1) * self.interleave_size \
                - 1

    # return the class
    return MainMemoryRange

# create the main memory range class
MainMemoryRange = MakeMainMemoryRangeClass(SYSCONFIG['sys_pxn_dram_ports'], SYSCONFIG["sys_pxn_dram_size"])

class CommandProcessor(object):
    CORE_ID = -1
    def initCore(self):
        """
        Initialize the tile's core
        """
        # create the core
        self.core = sst.Component("command_processor_pxn%d_pod%d" % (self.pxn, self.pod), "Drv.DrvCore")
        argv = []
        argv.append(arguments.program)
        argv.extend(arguments.argv)
        self.core.addParams({
            "verbose"   : arguments.verbose,
            "threads"   : 1,
            "clock"     : arguments.command_clock,
            "executable": arguments.with_command_processor,
            "argv" : ' '.join(argv), # cp its own exe as first arg, then same argv as the main program
            "max_idle" : 100//8, # turn clock offf after idle for 1 us
            "id"  : self.id,
            "pod" : self.pod,
            "pxn" : self.pxn,
            "phase_max" : arguments.stats_preallocated_phase,
        })
        self.core.addParams(SYSCONFIG)
        self.core.addParams(CORE_DEBUG)

        self.core_mem = self.core.setSubComponent("memory", "Drv.DrvStdMemory")
        self.core_mem.addParams({
            "verbose" : arguments.cp_verbose,
            "verbose_init" : arguments.cp_verbose_init,
            "verbose_requests" : arguments.cp_verbose_requests,
            "verbose_responses" : arguments.cp_verbose_responses,
        })

        self.core_iface = self.core_mem.setSubComponent("memory", "memHierarchy.standardInterface")
        self.core_iface.addParams({
            "verbose" : arguments.cp_verbose,
        })
        self.core_nic = self.core_iface.setSubComponent("memlink", "memHierarchy.MemNIC")
        self.core_nic.addParams({
            "group" : 0,
            "network_bw" : COMMAND_BW,
            "network_input_buffer_size" : arguments.network_onchip_buffer_size,
            "network_output_buffer_size" : arguments.network_onchip_buffer_size,
            "destinations" : "0,1,2",
            "verbose_level" : arguments.verbose_memory,
        })

    def __init__(self, pod=0, pxn=0):
        self.id = self.CORE_ID
        self.pod = pod
        self.pxn = pxn
        self.initCore()
