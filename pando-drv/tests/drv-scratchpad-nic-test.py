# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

import sst
import sys

DEBUG_MEM = 000
VERBOSE = 0

GROUPS = {
    "core" : 0,
    "scratchpad" : 1,
    "dram" : 2,
}

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
core_nic = iface.setSubComponent("memlink", "memHierarchy.MemNIC")
core_nic.addParams({
    "group" : GROUPS["core"],
    "network_bw" : "256GB/s",
    # this sets what valid destinations are for this link
    "destinations" : "%d,%d" % (GROUPS["scratchpad"], GROUPS["dram"]),
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
dram_nic = dram_memctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
dram_nic.addParams({
    "group" : GROUPS["dram"],
    "network_bw" : "256GB/s",
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
scratchpad_customcmdhandler.addParams({
    "verbose_level" : VERBOSE,
})
scratchpad_nic = scratchpad_memctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
scratchpad_nic.addParams({
    "group" : GROUPS["scratchpad"],
    "network_bw" : "256GB/s",
})

######################
# define the network #
######################
chiprtr = sst.Component("chiprtr", "merlin.hr_router")
chiprtr.addParams({
    "xbar_bw" : "256GB/s",
    "id" : "0",
    "input_buf_size" : "1KB",
    "num_ports" : "3",
    "flit_size" : "8B",
    "output_buf_size" : "1KB",
    "link_bw" : "256GB/s",
    # change this if we use a different network
    # topology; for now just one xbar router
    "topology" : "merlin.singlerouter"
})
chiprtr.setSubComponent("topology","merlin.singlerouter")

#########
# Links #
#########
link_core_chiprtr = sst.Link("link_core_chiprtr")
link_core_chiprtr.connect((core_nic, "port", "1ns"), (chiprtr, "port0", "1ns"))

link_scratchpad_chiprtr = sst.Link("link_scratchpad_chiprtr")
link_scratchpad_chiprtr.connect((scratchpad_nic, "port", "1ns"), (chiprtr, "port1", "1ns"))

link_dram_chiprtr = sst.Link("link_dram_chiprtr")
link_dram_chiprtr.connect((dram_nic, "port", "1ns"), (chiprtr, "port2", "1ns"))
