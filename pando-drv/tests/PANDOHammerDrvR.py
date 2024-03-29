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
from drv_pandohammer import *

print("""
PANDOHammerDrvR:
  program = {}
""".format(
    arguments.program
))

MakeTile = lambda id, pod, pxn : DrvRTile(id, pod, pxn)
MakePANDOHammer(MakeTile)

if (arguments.core_stats):
    DrvRTile.enableAllCoreStats()

sst.setStatisticLoadLevel(arguments.stats_load_level)
sst.setStatisticOutput("sst.statOutputCSV", {"filepath" : "stats.csv", "separator" : ","})
