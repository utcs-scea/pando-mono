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

core = sst.Component("core", "Drv.DrvCore")
core.addParams({
    "verbose" : VERBOSE,
    "debug_init" : True,
    "debug_clock" : True,
    "debug_requests" : True,
    "executable" : executable,
    "argv" : ' '.join(argv),
})
iface = core.setSubComponent("memory", "memHierarchy.standardInterface")
iface.addParams({
    "verbose" : VERBOSE,
})

memctrl = sst.Component("memctrl", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : VERBOSE,
    "verbose" : VERBOSE,
    "clock" : "1GHz",
    "addr_range_start" : 0,
    "addr_range_end" : 512*1024*1024-1,
    })
memory = memctrl.setSubComponent("backend", "Drv.DrvSimpleMemBackend")
memory.addParams({
    "access_time" : "1ns",
    "mem_size" : "512MiB",
    "verbose_level" : VERBOSE,
})
customcmdhandler = memctrl.setSubComponent("customCmdHandler", "Drv.DrvCmdMemHandler")
customcmdhandler.addParams({
    "verbose_level" : VERBOSE,
})

link_core_mem = sst.Link("link_core_mem")
link_core_mem.connect((iface, "port", "1ns"), (memctrl, "direct_link", "1ns"))
