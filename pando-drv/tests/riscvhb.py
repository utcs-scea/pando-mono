# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

import sst
import sys

MEM_BASE  = 0x00000000
MEM_SIZE  = 0x00020000

DRAM_BASE = 0x40000000
DRAM_SIZE = 0x40000000

STACK_BASE = MEM_BASE
STACK_SIZE = MEM_SIZE

size_to_str = lambda x: str(x) + "B"

# build the core
core = sst.Component("core", "Drv.RISCVCore")
core.addParams({
    "verbose" : 0,
    "clock" : "2GHz",
    "num_harts" : 1,
    "load" : 1,
    "program" : sys.argv[1],
    "sp" : "[0 {}]".format(STACK_BASE + STACK_SIZE - 8),
})
core_iface = core.setSubComponent("memory", "memHierarchy.standardInterface")
core_iface.addParams({
    "verbose" : 1,
})

# build the memory controller
scratchmemctrl = sst.Component("scratch", "memHierarchy.MemController")
scratchmemctrl.addParams({
    "clock" : "1GHz",
    "addr_range_start" : MEM_BASE+0,
    "addr_range_end"   : MEM_BASE+MEM_SIZE-1,
})
scratch = scratchmemctrl.setSubComponent("backend", "Drv.DrvSimpleMemBackend")
scratch.addParams({
    "access_time" : "2ns",
    "mem_size" : size_to_str(MEM_SIZE),
})

# build the dram memory controller
drammemctrl = sst.Component("dram", "memHierarchy.MemController")
drammemctrl.addParams({
    "clock" : "1GHz",
    "addr_range_start" : DRAM_BASE+0,
    "addr_range_end"   : DRAM_BASE+DRAM_SIZE-1,
})
dram = drammemctrl.setSubComponent("backend", "Drv.DrvSimpleMemBackend")
dram.addParams({
    "access_time" : "100ns",
    "mem_size" : size_to_str(DRAM_SIZE),
})

bus = sst.Component("bus", "memHierarchy.Bus")
bus.addParams({
    "bus_frequency" : "1GHz",
    "idle_max" : 0,
})

core_bus_link = sst.Link("core_bus_link")
core_bus_link.connect((core_iface, "port", "1ns"), (bus, "high_network_0", "1ns"))

bus_scratch_link = sst.Link("bus_scratch_link")
bus_scratch_link.connect((bus, "low_network_0", "1ns"), (scratchmemctrl, "direct_link", "1ns"))

bus_dram_link = sst.Link("bus_dram_link")
bus_dram_link.connect((bus, "low_network_1", "1ns"), (drammemctrl, "direct_link", "1ns"))
