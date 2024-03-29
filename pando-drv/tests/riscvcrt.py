# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

import sst
import sys

MEM_BASE = 0x00010000
MEM_SIZE = 0x00020000

STACK_BASE = 0x00020000
STACK_SIZE = 0x00010000

size_to_str = lambda x: str(x) + "B"

# build the core
core = sst.Component("core", "Drv.RISCVCore")
core.addParams({
    "verbose" : 0,
    "clock" : "2GHz",
    "num_harts" : 1,
    "load" : 1,
    "program" : sys.argv[1],
    "sp" : "[0 {}]".format(STACK_BASE+STACK_SIZE-8),
})
core_iface = core.setSubComponent("memory", "memHierarchy.standardInterface")
core_iface.addParams({
    "verbose" : 1,
})

# build the memory controller
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "clock" : "1GHz",
    "addr_range_start" : MEM_BASE+0,
    "addr_range_end"   : MEM_BASE+MEM_SIZE-1,
})
memory = memctrl.setSubComponent("backend", "Drv.DrvSimpleMemBackend")
memory.addParams({
    "access_time" : "100ns",
    "mem_size" : size_to_str(MEM_SIZE),
})

link = sst.Link("link")
link.connect((core_iface, "port", "1ns"), (memctrl, "direct_link", "1ns"))
