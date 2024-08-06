# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
# Included multi node simulation

########################################################################################################
# Diagram of this model:                                                                               #
# https://docs.google.com/presentation/d/1FnrAjOXJKo5vKgo7IkuSD7QT15aDAmJi5Pts6IQhkX8/edit?usp=sharing #
########################################################################################################
from drv import *
from drv_memory import *
from drv_tile import *

def MakePANDOHammer(make_tile):
    """
    @brief create a PANDOHammer model
    @param make_tile: a factory function that returns a tile object (a DrvX or DrvR tile)
    """
    PXNS = SYSCONFIG["sys_num_pxn"]
    PODS = SYSCONFIG["sys_pxn_pods"]
    CORES = SYSCONFIG["sys_pod_cores"]
    POD_L2_BANKS = SYSCONFIG["sys_pod_l2sp_banks"]
    PXN_MAINMEM_BANKS = SYSCONFIG["sys_pxn_dram_ports"]

    onchiprtr = []
    # build pxns
    for pxn in range(PXNS):
        # build the main memory banks
        mainmem_banks = []
        for i in range(PXN_MAINMEM_BANKS):
            mainmem_banks.append(MainMemoryBank(i,0,pxn))

        # build the network crossbar
        chiprtr = sst.Component("chiprtr%d" % pxn, "merlin.hr_router")
        chiprtr.addParams({
            # semantics parameters
            "id" : CHIPRTR_ID + pxn,
            "num_ports" : PODS * (CORES + POD_L2_BANKS) + (1 if arguments.with_command_processor else 0) + PXN_MAINMEM_BANKS + PXNS - 1, # If number of PXNS is equal to 1 we do not need additional port. Hence -1
            "topology" : "merlin.singlerouter",
            # performance models
            "xbar_bw" : (PODS * (CORES + POD_L2_BANKS) + (1 if arguments.with_command_processor else 0) + PXN_MAINMEM_BANKS + PXNS - 1) * arguments.network_base_bw,
            "link_bw" : (PODS * (CORES + POD_L2_BANKS) + (1 if arguments.with_command_processor else 0) + PXN_MAINMEM_BANKS + PXNS - 1) * arguments.network_base_bw,
            "flit_size" : "8B",
            "input_buf_size" : arguments.network_onchip_buffer_size,
            "output_buf_size" : arguments.network_onchip_buffer_size,
            'input_latency' : router_latency('chiprtr'),
            'output_latency' : router_latency('chiprtr'),
        })
        chiprtr.setSubComponent("topology","merlin.singlerouter")
        onchiprtr.append(chiprtr)

        # build pods
        for pod in range(PODS):
            # build the tiles
            tiles = []
            for i in range(CORES):
                tiles.append(make_tile(i,pod,pxn))

            tiles[0].markAsLoader()

            # build the shared memory
            l2_banks = []
            for i in range(POD_L2_BANKS):
                l2_banks.append(L2MemoryBank(i,pod,pxn))

            # wire up the tiles network
            base_core_no = pod*(CORES+POD_L2_BANKS)
            for (i, tile) in enumerate(tiles):
                bridge = sst.Component("bridge_%d_pod%d_pxn%d" % (i, pod, pxn), "merlin.Bridge")
                bridge.addParams({
                    "translator" : "memHierarchy.MemNetBridge",
                    "debug" : 1,
                    "debug_level" : 10,
                    "network_bw" : arguments.network_base_bw,
                    "network_input_buffer_size" : arguments.network_onchip_buffer_size,
                    "network_output_buffer_size" : arguments.network_onchip_buffer_size,
                })
                tile_bridge_link = sst.Link("tile_bridge_link_%d_pod%d_pxn%d" % (i, pod, pxn))
                tile_bridge_link.connect(
                    (bridge, "network0", link_latency('tile_bridge_link')),
                    (tile.tile_rtr, "port2", link_latency('tile_bridge_link'))
                )
                bridge_chiprtr_link = sst.Link("bridge_chiprtr_link_%d_pod%d_pxn%d" % (i, pod, pxn))
                bridge_chiprtr_link.connect(
                    (bridge, "network1", link_latency('bridge_chiprtr_link')),
                    (chiprtr, "port%d" % (base_core_no+i), link_latency('bridge_chiprtr_link'))
                )

            # wire up the shared memory
            base_l2_bankno = pod*(CORES+POD_L2_BANKS)+len(tiles)
            for (i, l2_bank) in enumerate(l2_banks):
                bridge = sst.Component("bridge_%d_pod%d_pxn%d" % (base_l2_bankno+i, pod, pxn), "merlin.Bridge")
                bridge.addParams({
                    "translator" : "memHierarchy.MemNetBridge",
                    "debug" : 1,
                    "debug_level" : 10,
                    "network_bw" : arguments.network_base_bw,
                    "network_input_buffer_size" : arguments.network_onchip_buffer_size,
                    "network_output_buffer_size" : arguments.network_onchip_buffer_size,
                })
                l2_bank_bridge_link = sst.Link("l2bank_bridge_link_%d_pod%d_pxn%d" % (i, pod, pxn))
                l2_bank_bridge_link.connect(
                    (bridge, "network0", link_latency('l2_bank_bridge_link')),
                    (l2_bank.mem_rtr, "port1", link_latency('l2_bank_bridge_link'))
                )
                bridge_chiprtr_link = sst.Link("bridge_chip_memrtr_link_%d_pod%d_pxn%d" % (i, pod, pxn))
                bridge_chiprtr_link.connect(
                    (bridge, "network1", link_latency('bridge_chiprtr_link')),
                    (chiprtr, "port%d" % (base_l2_bankno+i), link_latency('bridge_chiprtr_link'))
                )

        # wire up the main memory
        base_mainmem_bankno = PODS*(CORES+POD_L2_BANKS)
        for (i, mainmem_bank) in enumerate(mainmem_banks):
            bridge = sst.Component("mainmem_bridge_%d_pxn%d" % (i, pxn), "merlin.Bridge")
            bridge.addParams({
                "translator" : "memHierarchy.MemNetBridge",
                "debug" : 1,
                "debug_level" : 10,
                "network_bw" : arguments.network_base_bw,
                "network_input_buffer_size" : arguments.network_onchip_buffer_size,
                "network_output_buffer_size" : arguments.network_onchip_buffer_size,
            })
            mainmem_bank_bridge_link = sst.Link("mainmem_bank_bridge_link_%d_pxn%d" % (i, pxn))
            mainmem_bank_bridge_link.connect(
                (bridge, "network0", link_latency('mainmem_bank_bridge_link')),
                (mainmem_bank.mem_rtr, "port1", link_latency('mainmem_bank_bridge_link'))
            )
            bridge_chiprtr_link = sst.Link("bridge_chip_mainmem_memrtr_link_%d_pxn%d" % (i, pxn))
            bridge_chiprtr_link.connect(
                (bridge, "network1", link_latency('bridge_chiprtr_link')),
                (chiprtr, "port%d" % (base_mainmem_bankno+i), link_latency('bridge_chiprtr_link'))
            )

        # wire up the command processor
        if arguments.with_command_processor:
            command_processor = CommandProcessor(pod,pxn)
            chiprtr_command_processor_link = sst.Link("chiprtr_command_processor_link_pod%d_pxn%d" % (pod, pxn))
            chiprtr_command_processor_link.connect(
                (chiprtr, "port%d" % (PODS*(CORES+POD_L2_BANKS) + PXN_MAINMEM_BANKS), link_latency("chiprtr_command_processor_link")),
                (command_processor.core_nic, "port", link_latency("chiprtr_command_processor_link"))
            )

    # build off-chip network crossbar
    if (PXNS > 1):
        offchiprtr = sst.Component("offchiprtr", "merlin.hr_router")
        offchiprtr.addParams({
            # semantics parameters
            "id" : PXNS,
            "num_ports" : PXNS,
            "topology" : "merlin.singlerouter",
            # performance models
            "xbar_bw" : PXNS * arguments.network_base_bw,
            "link_bw" : PXNS * arguments.network_base_bw,
            "flit_size" : "8B",
            "input_buf_size" : arguments.network_offchip_buffer_size,
            "output_buf_size" : arguments.network_offchip_buffer_size,
            "input_latency" : router_latency('offchiprtr'),
            "output_latency" : router_latency('offchiprtr'),
        })
        offchiprtr.setSubComponent("topology","merlin.singlerouter")

        # wire up off chip and on chip rtrs
        for pxn in range(PXNS):
            shared_mem_port = PODS*(CORES+POD_L2_BANKS) + PXN_MAINMEM_BANKS + (1 if arguments.with_command_processor else 0)
            bridge = sst.Component("offchiprtr_bridge_pxn_%d" % pxn, "merlin.Bridge")
            bridge.addParams({
                "translator" : "memHierarchy.MemNetBridge",
                "debug" : 1,
                "debug_level" : 10,
                "network_bw" : arguments.network_base_bw,
                "network_input_buffer_size" : arguments.network_offchip_buffer_size,
                "network_output_buffer_size" : arguments.network_offchip_buffer_size,
            })
            onchiprtr_bridge_link = sst.Link("onchiprtr_bridge_link_pxn_%d" % pxn)
            onchiprtr_bridge_link.connect(
                (bridge, "network0", link_latency('onchiprtr_bridge_link')),
                (onchiprtr[pxn], "port%d" % shared_mem_port, link_latency('onchiprtr_bridge_link'))
            )
            offchiprtr_bridge_chiprtr_link = sst.Link("bridge_offchiprtr_link_pxn_%d" % pxn)
            offchiprtr_bridge_chiprtr_link.connect(
                (bridge, "network1", link_latency('offchiprtr_bridge_chiprtr_link')),
                (offchiprtr, "port%d" % (pxn), link_latency('offchiprtr_bridge_chiprtr_link'))
            )
    # done building model
    return
