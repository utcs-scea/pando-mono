# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

import sst
import sys
import argparse

MEM_BASE  = 0x00000000
MEM_SIZE  = 0x00020000

DRAM_BASE = 0x40000000
DRAM_SIZE = 0x40000000

STACK_BASE = MEM_BASE
STACK_SIZE = MEM_SIZE


SYSCONFIG = {
    "num_pxn"      : 1,
    "pxn_pods"     : 1,
    "pod_cores"    : 1,
}
SYSCONFIG = { "sys_" + k : v for k, v in SYSCONFIG.items() }

size_to_str = lambda x: str(x) + "B"

parser = argparse.ArgumentParser()
parser.add_argument("program", help="program to run")
parser.add_argument("--harts", type=int, default=1, help="number of harts")
parser.add_argument("--verbose-core", type=int, default=0, help="verbosity of core")
parser.add_argument("--dram-backend", type=str, default="simple", choices=['simple', 'ramulator'], help="backend timing model for DRAM")
args = parser.parse_args()

# set stack pointers
# divide stack space among harts
STACK_WORDS = STACK_SIZE // 8
HART_STACK_WORDS = STACK_WORDS // args.harts
HART_STACK_SIZE = HART_STACK_WORDS * 8

sp_v = ["{} {}".format(i, STACK_BASE + ((i+1)*HART_STACK_SIZE) - 8) for i in range(args.harts)]
sp_str = "[" + ", ".join(sp_v) + "]"

print(
    """
    Running {}:
    - harts: {}
    - sp: {}
    - verbose-core: {}
    """.format(args.program, args.harts, sp_str, args.verbose_core)
)
# build the core
core = sst.Component("core", "Drv.RISCVCore")
core.addParams({
    "verbose" : args.verbose_core,
    "clock" : "2GHz",
    "num_harts" : args.harts,
    "load" : 1,
    "program" : args.program,
    "sp" : sp_str,
    "core" : 0,
    "pod" : 0,
    "pxn" : 0,
})
core.addParams(SYSCONFIG)

core_iface = core.setSubComponent("memory", "memHierarchy.standardInterface")
core_iface.addParams({
    "verbose" : args.verbose_core,
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
    "verbose" : args.verbose_core,
})
scratchcmdhandler = scratchmemctrl.setSubComponent("customCmdHandler", "Drv.DrvCmdMemHandler")
scratchcmdhandler.addParams({
    "verbose_level" : args.verbose_core,
})

# build the dram memory controller
drammemctrl = sst.Component("dram", "memHierarchy.MemController")
drammemctrl.addParams({
    "clock" : "1GHz",
    "addr_range_start" : DRAM_BASE+0,
    "addr_range_end"   : DRAM_BASE+DRAM_SIZE-1,
})
if (args.dram_backend == "simple"):
    dram = drammemctrl.setSubComponent("backend", "Drv.DrvSimpleMemBackend")
    dram.addParams({
        "access_time" : "100ns",
        "mem_size" : size_to_str(DRAM_SIZE),
    })
elif (args.dram_backend == "ramulator"):
    dram = drammemctrl.setSubComponent("backend", "memHierarchy.ramulator")
    dram.addParams({
        "mem_size" : size_to_str(DRAM_SIZE),
        "configFile" : "/root/sst-ramulator-src/configs/hbm4-pando-config.cfg",
    })
dramcmdhandler = drammemctrl.setSubComponent("customCmdHandler", "Drv.DrvCmdMemHandler")
dramcmdhandler.addParams({
    "verbose_level" : args.verbose_core,
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
