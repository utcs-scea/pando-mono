# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington
import sst
from drv import *

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
        self.id = self.make_id(bank, *args, **kwargs)
        self.address_range = self.make_address_range(bank, *args, **kwargs)
        self.name = self.make_name(*args, **kwargs)
        self.memctrl = sst.Component("{}_memctrl_{}".format(self.name,self.id), "memHierarchy.MemController")
        self.memctrl.addParams({
            "clock" : "1GHz",
            "addr_range_start" : self.address_range.start,
            "addr_range_end"   : self.address_range.end,
            "interleave_size" :  str(self.address_range.interleave_size) + 'B',
            "interleave_step" : str(self.address_range.interleave_step) + 'B',
            "debug" : 1,
            "debug_level" : arguments.verbose_memory,
            "verbose" : arguments.verbose_memory,
        })
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
            "network_input_buffer_size" : "1MB",
            "network_output_buffer_size" : "1MB",
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
            "input_buf_size" : "1MB",
            "output_buf_size" : "1MB",
            'input_latency' : router_latency('mem_rtr'),
            'output_latency' : router_latency('mem_rtr'),
            "debug" : 1,
        })
        self.mem_rtr.setSubComponent("topology","merlin.singlerouter")
        # setup connection rtr <-> mem
        self.mem_nic_link = sst.Link("{}_nic_link_{}".format(self.name,self.id))
        self.mem_nic_link.connect(
            (self.nic, "port", link_latency('mem_nic_link')),
            (self.mem_rtr, "port0", link_latency('mem_nic_link')),
        )

        # print("{}: start={:016x} end={:016x}".format(
        #     self.name,
        #     self.address_range.start,
        #     self.address_range.end,
        # ))

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
            "access_time" : memory_latency('l2sp'),
            "mem_size" : L2SPRange.L2SP_BANK_SIZE_STR,
        })
        return backend

class MainMemoryBank(SharedMemoryBank):
    def __init__(self, bank, pod=0, pxn=0):
        super().__init__(bank, pod, pxn)

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
            # throughput for this backend is the mem_clock requests / second
            backend = self.memctrl.setSubComponent("backend", "Drv.DrvSimpleMemBackend")
            backend.addParams({
                "verbose_level" : arguments.verbose_memory,
                "access_time" : memory_latency('dram'),
                "mem_size" : MainMemoryRange.MAINMEM_BANK_SIZE_STR,
            })
            return backend
        elif (arguments.dram_backend == "ramulator"):
            backend = self.memctrl.setSubComponent("backend", "Drv.DrvRamulatorMemBackend")
            backend.addParams({
                "verbose_level" : arguments.verbose_memory,
                "configFile" : arguments.dram_backend_config,
                "mem_size" : MainMemoryRange.MAINMEM_BANK_SIZE_STR,
            })
            return backend
        elif (arguments.dram_backend == "dramsim3"):
            backend = self.memctrl.setSubComponent("backend", "Drv.DrvDRAMSim3MemBackend")
            backend.addParams({
                "verbose_level" : arguments.verbose_memory,
                "config_ini" : arguments.dram_backend_config,
                "mem_size" : MainMemoryRange.MAINMEM_BANK_SIZE_STR,
            })
            return backend
        else:
            raise Exception("Unknown DRAM backend: {}".format(arguments.dram_backend))
