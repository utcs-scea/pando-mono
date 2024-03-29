# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

import sst
import sys

VERBOSE = 0
VERBOSE_MEMCTRL = 0
CORE_DEBUG = {
    "init"      : False,
    "clock"     : False,
    "requests"  : False,
    "responses" : False,
    "loopback"  : False,
}

CORES = 64
THREADS = 16
DRAM_BASE = 0x80000000
L1SP_BASE = 0x00000000

SYSCONFIG = {
    "num_pxn"      : 1,
    "pxn_pods"     : 1,
    "pod_cores"    : CORES,
}

SYSCONFIG = { "sys_" + k : v for k, v in SYSCONFIG.items() }

if (len(sys.argv) < 2):
    print("ERROR: Must specify executable to run")
    exit(1)

executable = sys.argv[1]
argv = sys.argv[2:]

print("Drv Simulation: executable: %s, argv: %s" % (executable, argv))

class Tile(object):
    def __init__(self, id):
        # create the core
        self.core = sst.Component("core_%d" % id, "Drv.DrvCore")
        self.core.addParams({
            "verbose" : VERBOSE,
            "threads" : THREADS,
            "clock"   : "125MHz",
            "debug_init" : CORE_DEBUG["init"],
            "debug_clock" : CORE_DEBUG["clock"],
            "debug_requests" : CORE_DEBUG["requests"],
            "debug_responses" : CORE_DEBUG["responses"],
            "debug_loopback" : CORE_DEBUG["loopback"],
            "max_idle" : int(1000/8), # turn clock off after idle for 1 us
            "dram_base" : DRAM_BASE,
            "l1sp_base" : L1SP_BASE,
            "executable" : executable,
            "argv" : ' '.join(argv),
            "id" : id,
            "pod" : 0,
            "pxn" : 0,
        })
        self.core.addParams(SYSCONFIG)
        self.core_mem = self.core.setSubComponent("memory", "Drv.DrvStdMemory")
        self.core_iface = self.core_mem.setSubComponent("memory", "memHierarchy.standardInterface")
        self.core_iface.addParams({
            "verbose" : VERBOSE_MEMCTRL,
        })
        # create the scratchpad
        self.scratchpad_mectrl = sst.Component("scratchpad_mectrl_%d" % id, "memHierarchy.MemController")
        self.scratchpad_mectrl.addParams({
            "debug" : VERBOSE_MEMCTRL,
            "debug_level" : VERBOSE_MEMCTRL,
            "verbose" : VERBOSE_MEMCTRL,
            "clock" : "1GHz",
            "addr_range_start" : L1SP_BASE+(i+0)*4*1024+0,
            "addr_range_end" :   L1SP_BASE+(i+1)*4*1024-1,
        })
        # set the backend memory system to Drv special memory
        # (needed for AMOs)
        self.scratchpad = self.scratchpad_mectrl.setSubComponent("backend", "Drv.DrvSimpleMemBackend")
        self.scratchpad.addParams({
            "verbose_level" : VERBOSE_MEMCTRL,
            "access_time" : "1ns",
            "mem_size" : "4KiB",
        })
        # set the custom command handler
        # we need to use the Drv custom command handler
        # to handle our custom commands (AMOs)
        self.scratchpad_customcmdhandler = self.scratchpad_mectrl.setSubComponent("customCmdHandler", "Drv.DrvCmdMemHandler")
        self.scratchpad_customcmdhandler.addParams({
            "verbose_level" : VERBOSE_MEMCTRL,
        })

class SharedMemory(object):
    def __init__(self, id):
        self.memctrl = sst.Component("memctrl_%d" % id, "memHierarchy.MemController")
        self.memctrl.addParams({
            "clock" : "1GHz",
            "addr_range_start" : DRAM_BASE+(id+0)*512*1024*1024+0,
            "addr_range_end"   : DRAM_BASE+(id+1)*512*1024*1024-1,
            "debug" : 1,
            "debug_level" : VERBOSE_MEMCTRL,
            "verbose" : VERBOSE_MEMCTRL,
        })
        # set the backend memory system to Drv special memory
        # (needed for AMOs)
        self.memory = self.memctrl.setSubComponent("backend", "Drv.DrvSimpleMemBackend")
        self.memory.addParams({
            "verbose_level" : VERBOSE_MEMCTRL,
            "access_time" : "32ns",
            "mem_size" : "512MiB",
        })
        # set the custom command handler
        # we need to use the Drv custom command handler
        # to handle our custom commands (AMOs)
        self.customcmdhandler = self.memctrl.setSubComponent("customCmdHandler", "Drv.DrvCmdMemHandler")
        self.customcmdhandler.addParams({
            "verbose_level" : VERBOSE_MEMCTRL,
        })

# build the tiles
tiles = []
for i in range(CORES):
    tiles.append(Tile(i))

# build the shared memory
shared_memory = SharedMemory(0)

# build the bus
bus = sst.Component("bus", "memHierarchy.Bus")
bus.addParams({
    "bus_frequency" : "1GHz",
    "idle_max" : 0,
})

# wire up the bus
for (i, tile) in enumerate(tiles):
    core_bus_link = sst.Link("core_bus_link_%d" % i)
    spm_bus_link = sst.Link("spm_bus_link_%d" % i)
    core_bus_link.connect((tile.core_iface, "port", "1ns"), (bus, "high_network_%d" % i, "1ns"))
    spm_bus_link.connect((tile.scratchpad_mectrl, "direct_link", "1ns"), (bus, "low_network_%d" % i, "1ns"))

# wire up the shared memory
shm_bus_link = sst.Link("shm_bus_link")
shm_bus_link.connect((shared_memory.memctrl, "direct_link", "1ns"), (bus, "low_network_%d" % CORES, "1ns"))
