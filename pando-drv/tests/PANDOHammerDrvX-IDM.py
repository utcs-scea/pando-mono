# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington
# Copyright (c) 2023 University of Michigan

from drv import *
import sst
class SharedMemoryBank(object):
    NAME = "sharedmem"
    def __init__(self, bank, *args, **kwargs):
        """
        @brief Initialize a shared memory bank
        This constructor is the work horse of setting up the shared memory bank
        We leave a handful of functions to be implemented by the subclass

        id            : a unique identifier for this bank
        address_range : {L1SPRanage, L2SPRange, MainMemoryRange}
        name          : str
        memctrl       : sst.Component ("memHierarchy.MemController")
        memory        : sst.SubComponent ("memHierarchy.backend")
        """
        preload = None
        if "preload" in kwargs:
          preload = kwargs["preload"]
          del kwargs["preload"]

        self.id = self.make_id(bank, *args, **kwargs)
        self.address_range = self.make_address_range(bank, *args, **kwargs)
        self.name = self.make_name(*args, **kwargs)
        self.memctrl = sst.Component("{}_memctrl_{}".format(self.name,self.id), "memHierarchy.MemController")
        memargs = {
            "clock" : "1GHz",
            "addr_range_start" : self.address_range.start,
            "addr_range_end"   : self.address_range.end,
            "debug" : 1,
            "debug_level" : arguments.verbose_memory,
            "verbose" : arguments.verbose_memory,
        }

        if not preload is None:
          memargs["backing"] = "mmap"
          memargs["memory_file"] = preload
          print("  load {} at address 0x{:16x} ".format(preload, self.address_range.start))

        self.memctrl.addParams(memargs)
        # set the backend memory system to Drv special memory
        # (needed for AMOs)
        self.memory = self.make_backend(self.memctrl)
        # set the custom command handler
        # we need to use the Drv custom command handler
        # to handle our custom commands (AMOs)
        self.customcmdhandler = self.memctrl.setSubComponent("customCmdHandler", "Drv.DrvCmdMemHandler")
        self.customcmdhandler.addParams({
            "verbose_level" : arguments.verbose_memory,
        })
        # network interface
        self.nic = self.memctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
        self.nic.addParams({
            "group" : 2,
            "network_bw" : "256GB/s",
            "verbose_level": arguments.verbose_memory,
        })

        # create the tile network
        self.mem_rtr = sst.Component("{}_rtr_{}".format(self.name,self.id), "merlin.hr_router")
        self.mem_rtr.addParams({
            # semantics parameters
            "id" : SHAREDMEM_RTR_ID+self.id,
            "num_ports" : 2,
            "topology" : "merlin.singlerouter",
            # performance models
            "xbar_bw" : "1024GB/s",
            "link_bw" : "1024GB/s",
            "flit_size" : "8B",
            "input_buf_size" : "1KB",
            "output_buf_size" : "1KB",
            "debug" : 1,
        })
        self.mem_rtr.setSubComponent("topology","merlin.singlerouter")
        # setup connection rtr <-> mem
        self.mem_nic_link = sst.Link("{}_nic_link_{}".format(self.name,self.id))
        self.mem_nic_link.connect(
            (self.nic, "port", "1ns"),
            (self.mem_rtr, "port0", "1ns"),
        )


    def make_id(self, bank, *args, **kwargs):
        """
        @brief return a unique integer

        Subclass defines this.
        """
        raise NotImplementedError

    def make_address_range(self, bank, *args, **kwargs):
        """
        @brief return a L1SPRange, L2SPRange, or MainMemoryRange

        Subclass defines this
        """
        raise NotImplementedError

    def make_name(self, *args, **kwargs):
        """
        @brief return a string naming this bank

        Subclass defines this
        """
        raise NotImplementedError

    def make_backend(self, memctrl):
        """
        @brief return a sst.SubComponent ("memHierarchy.backend")

        Subclass defines this
        """
        raise NotImplementedError


class L2MemoryBank(SharedMemoryBank):
    def __init__(self, bank, pod=0, pxn=0):
        super().__init__(bank, pod, pxn)

    def make_id(self, bank, pod=0, pxn=0):
        """
        @brief return a unique integer
        """
        pod_shift = ADDR_POD_HI - ADDR_POD_LO+1
        pxn_shift = ADDR_PXN_LO - ADDR_PXN_LO+1 + pod_shift
        return bank + (pod << pod_shift) + (pxn << pxn_shift)

    def make_address_range(self, bank, pod=0, pxn=0):
        """
        @brief return a L1SPRange, L2SPRange, or MainMemoryRange
        """
        return L2SPRange(pxn, pod, bank)

    def make_name(self, pod=0, pxn=0):
        """
        @brief return a string naming this bank
        """
        return "l2mem_pxn{}_pod{}".format(pxn, pod)

    def make_backend(self, memctrl):
        """
        @brief return a sst.SubComponent ("memHierarchy.backend")
        """
        backend =  memctrl.setSubComponent("backend", "Drv.DrvSimpleMemBackend")
        backend.addParams({
            "verbose_level" : arguments.verbose_memory,
            "access_time" : "32ns",
            "mem_size" : L2SPRange.L2SP_BANK_SIZE_STR,
        })
        return backend

class MainMemoryBank(SharedMemoryBank):
    def __init__(self, bank, pod=0, pxn=0, preload = None):
        if preload is None:
          super().__init__(bank, pod, pxn)
        else:
          super().__init__(bank, pod, pxn, preload = preload)

    def make_id(self, bank, pod=0, pxn=0):
        """
        @brief return a unique integer
        """
        pod_shift = ADDR_POD_HI - ADDR_POD_LO+1
        pxn_shift = ADDR_PXN_LO - ADDR_PXN_LO+1 + pod_shift
        mainmem_shift = 1 + pxn_shift
        return bank + (1 << mainmem_shift) + (pod << pod_shift) + (pxn << pxn_shift)

        raise NotImplementedError

    def make_address_range(self, bank, pod=0, pxn=0):
        """
        @brief return a L1SPRange, L2SPRange, or MainMemoryRange
        """
        return MainMemoryRange(pxn, pod, bank)

    def make_name(self, pod=0, pxn=0):
        """
        @brief return a string naming this bank
        """
        return "mainmem_pxn{}_pod{}".format(pxn, pod)

    def make_backend(self, memctrl):
        """
        @brief return a sst.SubComponent ("memHierarchy.backend")
        """
        if (arguments.dram_backend == "simple"):
            backend = self.memctrl.setSubComponent("backend", "Drv.DrvSimpleMemBackend")
            backend.addParams({
                "verbose_level" : arguments.verbose_memory,
                "access_time" : "32ns",
                "mem_size" : MainMemoryRange.POD_MAINMEM_BANK_SIZE_STR,
            })
            return backend
        elif (arguments.dram_backend == "ramulator"):
            backend = self.memctrl.setSubComponent("backend", "Drv.DrvRamulatorMemBackend")
            backend.addParams({
                "verbose_level" : arguments.verbose_memory,
                "configFile" : arguments.dram_backend_config,
                "mem_size" : MainMemoryRange.POD_MAINMEM_BANK_SIZE_STR,
            })
            return backend
        else:
            raise Exception("Unknown DRAM backend: {}".format(arguments.dram_backend))
import sys

# for drvx we set the core clock to 125MHz (assumption is 1/8 ops are memory)
SYSCONFIG["sys_core_clock"] = "125MHz"

# SYSCONFIG["sys_pod_cores"] = 2
# SYSCONFIG["sys_core_threads"] = 1

import pathlib
projRoot = pathlib.Path(__file__).parent.parent

image = str(projRoot/'examples/IDM/inputs/Data01CSR.idmimg')

print("""
PANDOHammerDrvX:
  program = {}
""".format(
    arguments.program
))

class Tile(object):
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
        self.scratchpad_mectrl = sst.Component("scratchpad_mectrl_%d" % self.id, "memHierarchy.MemController")
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
            "access_time" : "1ns",
            "mem_size" : "4KiB",
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
        self.tile_rtr = sst.Component("tile_rtr_%d" % self.id, "merlin.hr_router")
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
            "debug" : 1,
        })

        # setup connection rtr <-> core
        self.tile_rtr.setSubComponent("topology","merlin.singlerouter")
        self.core_nic_link = sst.Link("core_nic_link_%d" % self.id)
        self.core_nic_link.connect(
            (self.core_nic, "port", "1ns"),
            (self.tile_rtr, "port0", "1ns")
        )

        # setup connection rtr <-> scratchpad
        self.scratchpad_nic_link = sst.Link("scratchpad_nic_link_%d" % self.id)
        self.scratchpad_nic_link.connect(
            (self.scratchpad_nic, "port", "1ns"),
            (self.tile_rtr, "port1", "1ns")
        )

    def initCore(self):
        """
        Initialize the tile's core
        """
        # create the core
        self.core = sst.Component("core_%d" % self.id, "Drv.DrvCore")
        self.core.addParams({
            "verbose"   : arguments.verbose,
            "threads"   : SYSCONFIG["sys_core_threads"],
            "clock"     : SYSCONFIG["sys_core_clock"],
            "executable": arguments.program,
            "argv" : ' '.join(arguments.argv),
            "max_idle" : 100//8, # turn clock offf after idle for 1 us
            "id"  : self.id,
            "pod" : self.pod,
            "pxn" : self.pxn,
        })
        self.core.addParams(SYSCONFIG)
        self.core.addParams(CORE_DEBUG)

        self.core_mem = self.core.setSubComponent("memory", "Drv.DrvStdMemory")
        self.core_mem.addParams({
            "verbose" : arguments.verbose,
        })

        self.core_iface = self.core_mem.setSubComponent("memory", "memHierarchy.standardInterface")
        self.core_iface.addParams({
            "verbose" : arguments.verbose,
        })
        self.core_nic = self.core_iface.setSubComponent("memlink", "memHierarchy.MemNIC")
        self.core_nic.addParams({
            "group" : 0,
            "network_bw" : "1024GB/s",
            "destinations" : "1,2",
            "verbose_level" : arguments.verbose_memory,
        })

    def __init__(self, id, pod=0, pxn=0):
        self.id = id
        self.pod = pod
        self.pxn = pxn
        self.l1sprange = L1SPRange(self.pxn, self.pod, self.id >> 3, self.id & 0x7)
        """
        Create a tile with the given ID, pod, and PXN.
        """
        self.initCore()
        self.initMem()
        self.initRtr()


# build the tiles
tiles = []
CORES = SYSCONFIG["sys_pod_cores"]
for i in range(CORES):
    tiles.append(Tile(i))

tiles[0].markAsLoader()

# build the shared memory
POD_L2_BANKS = SYSCONFIG["sys_pod_l2_banks"]
l2_banks = []
for i in range(POD_L2_BANKS):
    l2_banks.append(L2MemoryBank(i))

# build the main memory banks
POD_MAINMEM_BANKS = SYSCONFIG["sys_pod_dram_ports"]
mainmem_banks = []
# mainmem_banks.append(MainMemoryBank(0, preload=image))

for i in range(POD_MAINMEM_BANKS - 2):
    mainmem_banks.append(MainMemoryBank(i))

mainmem_banks.append(MainMemoryBank(POD_MAINMEM_BANKS - 2, preload=image))
mainmem_banks.append(MainMemoryBank(POD_MAINMEM_BANKS - 1, preload=image))


# for m in mainmem_banks:
#     print("0x{:16x}".format(m.address_range.start))



# build the network crossbar
chiprtr = sst.Component("chiprtr", "merlin.hr_router")
chiprtr.addParams({
    # semantics parameters
    "id" : CHIPRTR_ID,
    "num_ports" : CORES+POD_L2_BANKS+POD_MAINMEM_BANKS,
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
        "debug" : 1,
        "debug_level" : 10,
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
base_l2_bankno = len(tiles)
for (i, l2_bank) in enumerate(l2_banks):
    bridge = sst.Component("bridge_%d" % (base_l2_bankno+i), "merlin.Bridge")
    bridge.addParams({
        "translator" : "memHierarchy.MemNetBridge",
        "debug" : 1,
        "debug_level" : 10,
        "network_bw" : "256GB/s",
        })
    l2_bank_bridge_link = sst.Link("l2bank_bridge_link_%d" % i)
    l2_bank_bridge_link.connect(
        (bridge, "network0", "1ns"),
        (l2_bank.mem_rtr, "port1", "1ns")
    )
    bridge_chiprtr_link = sst.Link("bridge_chip_memrtr_link_%d" %i)
    bridge_chiprtr_link.connect(
        (bridge, "network1", "1ns"),
        (chiprtr, "port%d" % (base_l2_bankno+i), "1ns")
    )

# wire up the main memory
base_mainmem_bankno = base_l2_bankno + len(l2_banks)
for (i, mainmem_bank) in enumerate(mainmem_banks):
    bridge = sst.Component("mainmem_bridge_%d" % i, "merlin.Bridge")
    bridge.addParams({
        "translator" : "memHierarchy.MemNetBridge",
        "debug" : 1,
        "debug_level" : 10,
        "network_bw" : "256GB/s",
    })
    mainmem_bank_bridge_link = sst.Link("mainmem_bank_bridge_link_%d" % i)
    lat = '1ns'
    if i == len(mainmem_banks) - 1:
      lat = '1us'
    mainmem_bank_bridge_link.connect(
        (bridge, "network0", lat),
        (mainmem_bank.mem_rtr, "port1", "1ns")
    )
    bridge_chiprtr_link = sst.Link("bridge_chip_mainmem_memrtr_link_%d" %i)
    bridge_chiprtr_link.connect(
        (bridge, "network1", "1ns"),
        (chiprtr, "port%d" % (base_mainmem_bankno+i), "1ns")
    )
