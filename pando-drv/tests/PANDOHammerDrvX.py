# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
# Included multi node simulation

########################################################################################################
# Diagram of this model:                                                                               #
# https://docs.google.com/presentation/d/1ekm0MbExI1PKca5tDkSGEyBi-_0000Ro9OEaBLF-rUQ/edit?usp=sharing #
########################################################################################################
from drv import *
from drv_memory import *
from drv_tile import *
from drv_pandohammer import *

print("""
PANDOHammerDrvX: program = {}
""".format(
    arguments.program
))

MakeTile = lambda id, pod, pxn : DrvXTile(id, pod, pxn)
MakePANDOHammer(MakeTile)

if (arguments.core_stats):
    DrvXTile.enableAllCoreStats()
if (arguments.all_stats):
    sst.enableAllStatisticsForAllComponents()
if (arguments.perf_stats):
    sst.enableStatisticsForComponentType(
        "Drv.DrvCore",
        ["total_stall_cycles_when_ready",
        "total_stall_cycles",
        "total_busy_cycles",
        "phase_comp_stall_cycles_when_ready",
        "phase_comm_stall_cycles_when_ready",
        "phase_stall_cycles",
        "phase_busy_cycles"])
    sst.enableStatisticsForComponentType(
        "memHierarchy.MemNIC",
        ["send_bit_count",
        "recv_bit_count",
        "output_port_stalls",
        "idle_time"])
    sst.enableStatisticsForComponentType(
        "merlin.hr_router",
        ["send_bit_count",
        "recv_bit_count",
        "output_port_stalls",
        "idle_time"])
    sst.enableStatisticsForComponentType(
        "merlin.Bridge",
        ["send_bit_count",
        "recv_bit_count",
        "output_port_stalls",
        "idle_time"])

sst.setStatisticLoadLevel(arguments.stats_load_level)
sst.setStatisticOutput("sst.statOutputCSV", {"filepath" : "stats.csv", "separator" : ","})
