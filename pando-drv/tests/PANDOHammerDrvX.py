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

#if (arguments.core_stats):
#    DrvXTile.enableAllCoreStats()

sst.setStatisticLoadLevel(5)
sst.enableAllStatisticsForAllComponents()

#sst.setStatisticLoadLevel(arguments.stats_load_level)
sst.setStatisticOutput("sst.statOutputCSV", {"filepath" : "stats.csv", "separator" : ","})
