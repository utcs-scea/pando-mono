# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

import sst
import sys

VERBOSE = 0
CORES = 1
THREADS = 1

#executable = sys.argv[1]
if (len(sys.argv) < 2):
    print("ERROR: Must specify executable to run")
    exit(1)

executable = sys.argv[1]
argv = sys.argv[2:]

print("Drv Simulation: executable: %s, argv: %s" % (executable, argv))

# build a a shared memory
memctrl = sst.Component("memctrl", "memHierarchy.MemController")
memctrl.addParams({
    "clock" : "1GHz",
    "addr_range_start" : 0,
    "addr_range_end" : 512*1024*1024-1,
    "debug" : 1,
    "debug_level" : VERBOSE,
    "verbose" : VERBOSE,
})
# set the backend memory system
memory = memctrl.setSubComponent("backend", "Drv.DrvSimpleMemBackend")
# we need to use the Drv backend memory system
# for Drv specific memory commands (AMOs)
#
# this is a subclass of the simpleMem backend
# with an overrided issueCustomRequest method
memory.addParams({
    "access_time" : "1ns",
    "mem_size" : "512MiB",
})
# set the custom command handler
# we need to use the Drv custom command handler
# for Drv specific memory commands (AMOs)
customcmdhadler = memctrl.setSubComponent("customCmdHandler", "Drv.DrvCmdMemHandler")
customcmdhadler.addParams({
    "verbose_level" : VERBOSE,
})

# build a bus
bus = sst.Component("bus", "memHierarchy.Bus")
bus.addParams({
    "bus_frequency" : "1GHz",
    "idle_max" : 0,
})

# connect the bus to the memory
bus_memctrl_link = sst.Link("bus_memctrl_link")
bus_memctrl_link.connect(
    (bus, "low_network_0", "1ns"),
    (memctrl, "direct_link", "1ns")
)

# build the cores
cores = []
for i in range(CORES):
    core = sst.Component("core_%d" % i, "Drv.DrvCore")
    core.addParams({
        "verbose" : VERBOSE,
        "threads" : THREADS,
        "debug_init" : True,
        "debug_clock" : True,
        "debug_requests" : True,
        "debug_response" : True,
        "executable" : executable,
        "argv" : ' '.join(argv),
        "id" : i,
    })
    cores.append(core)

# connect the cores to the bus
for (idx, core) in enumerate(cores):
    iface = core.setSubComponent("memory", "memHierarchy.standardInterface")
    iface.addParams({
        "verbose" : VERBOSE,
    })
    core_bus_link = sst.Link("core_bus_link_%d" % idx)
    core_bus_link.connect(
        (iface, "port", "1ns"),
        (bus, "high_network_%d" % idx, "1ns")
    )
