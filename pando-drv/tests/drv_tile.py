# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
# Included multi node simulation

import sst
from drv import *

class Tile(object):
    """
    A Drv tile. A tile has a router, a scratchpad, and a core.
    """
    def l1sp_start(self):
        return self.l1sprange.start

    def l1sp_end(self):
        return self.l1sprange.end

    def markAsLoader(self):
        """
        Set this tile as responsible for loading the executable.
        """
        self.core.addParams({"load" : 1})

    def initMem(self):
        """
        Initialize the tile's scratchpad memory
        """
        # create the scratchpad
        self.scratchpad_mectrl = sst.Component("scratchpad_mectrl_%d_pod%d_pxn%d" % (self.id, self.pod, self.pxn),
                                               "memHierarchy.MemController")
        self.scratchpad_mectrl.addParams({
            "debug" : 0,
            "debug_level" : 0,
            "verbose" : 0,
            "clock" : "1GHz",
            "addr_range_start" : self.l1sp_start(),
            "addr_range_end" :   self.l1sp_end(),
        })
        # set the backend memory system to Drv special memory
        # (needed for AMOs)
        self.scratchpad = self.scratchpad_mectrl.setSubComponent("backend", "Drv.DrvSimpleMemBackend")
        self.scratchpad.addParams({
            "verbose_level" : arguments.verbose_memory,
            "access_time" : memory_latency('l1sp'),
            "mem_size" : L1SPRange.L1SP_SIZE_STR,
        })

        # set the custom command handler
        # we need to use the Drv custom command handler
        # to handle our custom commands (AMOs)
        self.scratchpad_customcmdhandler = self.scratchpad_mectrl.setSubComponent("customCmdHandler", "Drv.DrvCmdMemHandler")
        self.scratchpad_customcmdhandler.addParams({
            "verbose_level" : arguments.verbose_memory,
        })
        self.scratchpad_nic = self.scratchpad_mectrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
        self.scratchpad_nic.addParams({
            "group" : 1,
            "network_bw" : "1024GB/s",
        })

    def initRtr(self):
        """
        Initialize the tile's router
        """
        # create the tile network
        self.tile_rtr = sst.Component("tile_rtr_%d_pod%d_pxn%d" % (self.id, self.pod, self.pxn), "merlin.hr_router")
        self.tile_rtr.addParams({
            # semantics parameters
            "id" : COMPUTETILE_RTR_ID + self.id,
            "num_ports" : 3,
            "topology" : "merlin.singlerouter",
            # performance models
            "xbar_bw" : "1024GB/s",
            "link_bw" : "1024GB/s",
            "flit_size" : "8B",
            "input_buf_size" : "1KB",
            "output_buf_size" : "1KB",
            'input_latency' : router_latency('tile_rtr'),
            'output_latency' : router_latency('tile_rtr'),
            "debug" : 1,
        })

        # setup connection rtr <-> core
        self.tile_rtr.setSubComponent("topology","merlin.singlerouter")
        self.core_nic_link = sst.Link("core_nic_link_%d_pod%d_pxn%d" % (self.id, self.pod, self.pxn))
        self.core_nic_link.connect(
            (self.core_nic, "port", link_latency('core_nic_link')),
            (self.tile_rtr, "port0", link_latency('core_nic_link'))
        )

        # setup connection rtr <-> scratchpad
        self.scratchpad_nic_link = sst.Link("scratchpad_nic_link_%d_pod%d_pxn%d" % (self.id, self.pod, self.pxn))
        self.scratchpad_nic_link.connect(
            (self.scratchpad_nic, "port", link_latency('scratchpad_nic_link')),
            (self.tile_rtr, "port1", link_latency('scratchpad_nic_link'))
        )

    def initCore(self):
        """
        Initialize the tile's core
        Subclasses should define this
        """
        raise NotImplementedError

    def x(self):
        return self.id & 0x7

    def y(self):
        return (self.id >> 3) & 0x7

    def __init__(self, id, pod=0, pxn=0):
        """
        Create a tile with the given ID, pod, and PXN.
        """
        self.id = id
        self.pod = pod
        self.pxn = pxn
        self.l1sprange = L1SPRange(self.pxn, self.pod, self.y(), self.x())
        self.initCore()
        self.initMem()
        self.initRtr()

class DrvXTile(Tile):
    TILES = []
    """
    A tile with a DrvX core
    """
    def __init__(self, id, pod=0, pxn=0):
        """
        Create a tile with the given ID, pod, and PXN.
        """
        super().__init__(id, pod, pxn)
        self.TILES += [self]

    @classmethod
    def enableAllCoreStats(clss):
        sst.enableAllStatisticsForComponentType("Drv.DrvCore")
        for tile in clss.TILES:
            tile.core.enableStatistics(['tag_cycles'],
                                       {'type': 'sst.HistogramStatistic',
                                        'minvalue' : '0',
                                        'binwidth' : '1',
                                        'numbins' : '4',
                                        'IncludeOoutOfBounds' : '1'})

    def initCore(self):
        """
        Initialize the tile's core
        """
        # create the core
        self.core = sst.Component("core_%d_pod%d_pxn%d" % (self.id, self.pod, self.pxn), "Drv.DrvCore")
        self.core.addParams({
            "verbose"   : arguments.verbose,
            "threads"   : SYSCONFIG["sys_core_threads"],
            "executable": arguments.program,
            "argv" : ' '.join(arguments.argv),
            "max_idle" : KNOBS["core_max_idle"],
            "id"  : self.id,
            "pod" : self.pod,
            "pxn" : self.pxn,
            "phase_max" : arguments.stats_preallocated_phase,
            "stack_in_l1sp" : arguments.drvx_stack_in_l1sp,
        })

        if SYSCONFIG["sys_core_clock"]:
            self.core.addParams({
                "clock" : SYSCONFIG["sys_core_clock"],
            })

        self.core.addParams(SYSCONFIG)
        self.core.addParams(CORE_DEBUG)

        self.core_mem = self.core.setSubComponent("memory", "Drv.DrvStdMemory")
        self.core_mem.addParams({
            "verbose"           : arguments.verbose,
            "verbose_init"      : arguments.debug_init,
            "verbose_requests"  : arguments.debug_requests,
            "verbose_responses" : arguments.debug_responses,
        })

        self.core_iface = self.core_mem.setSubComponent("memory", "memHierarchy.standardInterface")
        self.core_iface.addParams({
            "verbose" : arguments.verbose,
        })
        self.core_nic = self.core_iface.setSubComponent("memlink", "memHierarchy.MemNIC")
        self.core_nic.addParams({
            "group" : 0,
            "network_bw" : "1024GB/s",
            "destinations" : "0,1,2",
            "verbose_level" : arguments.verbose_memory,
        })

class DrvRTile(Tile):
    """
    A tile with a DrvR core
    """
    def __init__(self, id, pod=0, pxn=0):
        """
        Create a tile with the given ID, pod, and PXN.
        """
        super().__init__(id, pod, pxn)

    @classmethod
    def enableAllCoreStats(clss):
        sst.enableAllStatisticsForComponentType("Drv.RISCVCore")

    def initCore(self):
        """
        Initialize the tile's core
        """
        # create the core
        self.core = sst.Component("core_%d_pod%d_pxn%d" % (self.id, self.pod, self.pxn), "Drv.RISCVCore")
        self.core.addParams({
            "verbose"   : arguments.verbose,
            "num_harts" : SYSCONFIG["sys_core_threads"],
            "program" : arguments.program,
            "release_reset" : 10000,
            #"argv" : ' '.join(argv), @ todo, make this work
            "core": self.id,
            "pod" : self.pod,
            "pxn" : self.pxn,
        })

        if SYSCONFIG["sys_core_clock"]:
            self.core.addParams({
                "clock" : SYSCONFIG["sys_core_clock"],
            })

        self.core.addParams(SYSCONFIG)
        self.core.addParams(CORE_DEBUG)
        self.core_iface = self.core.setSubComponent("memory", "memHierarchy.standardInterface")
        self.core_iface.addParams({
            "verbose" : arguments.verbose,
        })
        self.core_nic = self.core_iface.setSubComponent("memlink", "memHierarchy.MemNIC")
        self.core_nic.addParams({
            "group" : 0,
            "network_bw" : "1024GB/s",
            "destinations" : "0,1,2",
            "verbose_level" : arguments.verbose_memory,
        })
        self.initSP()

    def initSP(self):
        """
        Initialize the stack pointer"
        """
        # set stack pointers
        stack_base = self.l1sp_start()
        stack_bytes = self.l1sp_end() - self.l1sp_start() + 1
        stack_words = stack_bytes // 8
        thread_stack_words = stack_words // SYSCONFIG["sys_core_threads"]
        thread_stack_bytes = thread_stack_words * 8
        # build a string of stack pointers
        # to pass a parameter to the core
        sp_v = ["{} {}".format(i, stack_base + ((i+1)*thread_stack_bytes) - 8) for i in range(SYSCONFIG["sys_core_threads"])]
        sp_str = "[" + ", ".join(sp_v) + "]"
        self.core.addParams({"sp" : sp_str})
