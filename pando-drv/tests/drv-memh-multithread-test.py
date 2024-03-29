# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

import sst
import sys

#executable = sys.argv[1]
if (len(sys.argv) < 2):
    print("ERROR: Must specify executable to run")
    exit(1)

executable = sys.argv[1]

core = sst.Component("core", "Drv.DrvCore")
core.addParams({
    "verbose" : 0,
    "debug_init" : True,
    "debug_clock" : True,
    "executable" : executable,
    "threads" : 16,
})
iface = core.setSubComponent("memory", "memHierarchy.standardInterface")
iface.addParams({
    "verbose" : 0,
})

memctrl = sst.Component("memctrl", "memHierarchy.MemController")
memctrl.addParams({
    "clock" : "1GHz",
    "addr_range_start" : 0,
    "addr_range_end" : 512*1024*1024-1,
})
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "access_time" : "1ns",
    "mem_size" : "512MiB",
})
link_core_mem = sst.Link("link_core_mem")
link_core_mem.connect(
    (iface, "port", "1ns"),
    (memctrl, "direct_link", "1ns")
)
