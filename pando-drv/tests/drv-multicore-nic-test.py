# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

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
THREADS = 32
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

print("Drv Simulation: cores = %d, threads = %d, executable: %s, argv: %s" % (CORES, THREADS, executable, argv))

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
            "verbose" : VERBOSE,
        })
        self.core_nic = self.core_iface.setSubComponent("memlink", "memHierarchy.MemNIC")
        self.core_nic.addParams({
            "group" : 0,
            "network_bw" : "1024GB/s",
            "destinations" : "1,2",
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
            "verbose_level" : VERBOSE,
            "access_time" : "1ns",
            "mem_size" : "4KiB",
        })
        # set the custom command handler
        # we need to use the Drv custom command handler
        # to handle our custom commands (AMOs)
        self.scratchpad_customcmdhandler = self.scratchpad_mectrl.setSubComponent("customCmdHandler", "Drv.DrvCmdMemHandler")
        self.scratchpad_customcmdhandler.addParams({
            "verbose_level" : VERBOSE,
        })
        self.scratchpad_nic = self.scratchpad_mectrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
        self.scratchpad_nic.addParams({
            "group" : 1,
            "network_bw" : "1024GB/s",
        })
        # create the tile network
        self.tile_rtr = sst.Component("tile_rtr_%d" % id, "merlin.hr_router")
        self.tile_rtr.addParams({
            # semantics parameters
            "id" : id,
            "num_ports" : 3,
            "topology" : "merlin.singlerouter",
            # performance models
            "xbar_bw" : "1024GB/s",
            "link_bw" : "1024GB/s",
            "flit_size" : "8B",
            "input_buf_size" : "1KB",
            "output_buf_size" : "1KB",
        })
        self.tile_rtr.setSubComponent("topology","merlin.singlerouter")
        self.core_nic_link = sst.Link("core_nic_link_%d" % id)
        self.core_nic_link.connect(
            (self.core_nic, "port", "1ns"),
            (self.tile_rtr, "port0", "1ns")
        )
        self.scratchpad_nic_link = sst.Link("scratchpad_nic_link_%d" % id)
        self.scratchpad_nic_link.connect(
            (self.scratchpad_nic, "port", "1ns"),
            (self.tile_rtr, "port1", "1ns")
        )

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
            "verbose_level" : VERBOSE,
            "access_time" : "32ns",
            "mem_size" : "512MiB",
        })
        # set the custom command handler
        # we need to use the Drv custom command handler
        # to handle our custom commands (AMOs)
        self.customcmdhandler = self.memctrl.setSubComponent("customCmdHandler", "Drv.DrvCmdMemHandler")
        self.customcmdhandler.addParams({
            "verbose_level" : VERBOSE,
        })
        # network interface
        self.nic = self.memctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
        self.nic.addParams({
            "group" : 2,
            "network_bw" : "256GB/s",
        })

# build the tiles
tiles = []
for i in range(CORES):
    tiles.append(Tile(i))

# build the shared memory
shared_memory = SharedMemory(0)

# build the network crossbar
chiprtr = sst.Component("chiprtr", "merlin.hr_router")
chiprtr.addParams({
    # semantics parameters
    "id" : len(tiles),
    "num_ports" : len(tiles)+1,
    "topology" : "merlin.singlerouter",
    # performance models
    "xbar_bw" : "256GB/s",
    "link_bw" : "256GB/s",
    "flit_size" : "8B",
    "input_buf_size" : "1KB",
    "output_buf_size" : "1KB",
})
chiprtr.setSubComponent("topology","merlin.singlerouter")

# wire up the tiles network
for (i, tile) in enumerate(tiles):
    bridge = sst.Component("bridge_%d" % i, "merlin.Bridge")
    bridge.addParams({
        "translator" : "memHierarchy.MemNetBridge",
        "debug" : VERBOSE,
        "debug_level" : VERBOSE,
        "network_bw" : "256GB/s",
    })
    tile_bridge_link = sst.Link("tile_bridge_link_%d" % i)
    tile_bridge_link.connect(
        (bridge, "network0", "1ns"),
        (tile.tile_rtr, "port2", "1ns")
    )
    bridge_chiprtr_link = sst.Link("bridge_chiprtr_link_%d" % i)
    bridge_chiprtr_link.connect(
        (bridge, "network1", "1ns"),
        (chiprtr, "port%d" % i, "1ns")
    )

# wire up the shared memory
mem_rtr_link = sst.Link("mem_rtr_link")
mem_rtr_link.connect(
    (shared_memory.nic, "port", "1ns"),
    (chiprtr, "port%d" % len(tiles), "1ns")
)
