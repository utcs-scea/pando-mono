# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

import sst
import sys

DEBUG_MEM = 0
VERBOSE = 0

if (len(sys.argv) < 2):
    print("ERROR: Must specify executable to run")
    exit(1)

executable = sys.argv[1]
argv = sys.argv[2:]

############
# the core #
############
core = sst.Component("core", "Drv.DrvCore")
core.addParams({
    "verbose" : VERBOSE,
    "debug_init" : True,
    "debug_clock" : True,
    "debug_requests" : True,
    "executable" : executable,
    "argv" : ' '.join(argv),
    "id" : 0,
})
iface = core.setSubComponent("memory", "memHierarchy.standardInterface")
iface.addParams({
    "verbose" : VERBOSE,
})

##################
# the memory bus #
##################
mem_bus = sst.Component("mem_bus", "memHierarchy.Bus")
mem_bus.addParams({
    "bus_frequency" : "1GHz",
    "idle_max" : 0,
})

###############
# Dram memory #
###############
dram_memctrl = sst.Component("dram_memctrl", "memHierarchy.MemController")
dram_memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : VERBOSE,
    "verbose" : VERBOSE,
    "clock" : "1GHz",
    "addr_range_start" : 4*1024+0,
    "addr_range_end" :   8*1024-1,
    })
dram = dram_memctrl.setSubComponent("backend", "Drv.DrvSimpleMemBackend")
dram.addParams({
    "access_time" : "1ns",
    "mem_size" : "4KiB",
    "verbose_level" : VERBOSE,
})
dram_customcmdhandler = dram_memctrl.setSubComponent("customCmdHandler", "Drv.DrvCmdMemHandler")
dram_customcmdhandler.addParams({
    "verbose_level" : VERBOSE,
})

#####################
# Scratchpad memory #
#####################
scratchpad_memctrl = sst.Component("scratchpad_memctrl", "memHierarchy.MemController")
scratchpad_memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : VERBOSE,
    "verbose" : VERBOSE,
    "clock" : "1GHz",
    "addr_range_start" : 0*1024+0,
    "addr_range_end" :   4*1024-1,
})
scratchpad = scratchpad_memctrl.setSubComponent("backend", "Drv.DrvSimpleMemBackend")
scratchpad.addParams({
    "access_time" : "1ns",
    "mem_size" : "4KiB"
})
scratchpad_customcmdhandler = scratchpad_memctrl.setSubComponent("customCmdHandler", "Drv.DrvCmdMemHandler")

#########
# Links #
#########
link_core_bus = sst.Link("link_core_bus")
link_core_bus.connect((iface, "port", "1ns"), (mem_bus, "high_network_0", "1ns"))
link_bus_dram = sst.Link("link_bus_dram")
link_bus_dram.connect((mem_bus, "low_network_0", "1ns"), (dram_memctrl, "direct_link", "1ns"))
link_bus_scratchpad = sst.Link("link_bus_scratchpad")
link_bus_scratchpad.connect((mem_bus, "low_network_1", "1ns"), (scratchpad_memctrl, "direct_link", "1ns"))
