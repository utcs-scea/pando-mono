# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

import sys
import csv

if len(sys.argv) != 6:
    print("usage: python3 comparison.py (path to stat file) (number of pxns) (cores per pxn) (harts per core) (phase)")
    sys.exit(0)

statFile = sys.argv[1]
host = int(sys.argv[2])
core = int(sys.argv[3])
hart = int(sys.argv[4])
phase = sys.argv[5]

stall_cycles = []
busy_cycles = []
total_cycles = []
stall_cycles_when_thread_ready = []
stall_cycles_when_thread_ready_avg = []

for host_id in range(host):
    stall_cycles.append([])
    busy_cycles.append([])
    total_cycles.append([])
    stall_cycles_when_thread_ready.append([])
    stall_cycles_when_thread_ready_avg.append([])

    for core_id in range(core):
        stall_cycles[host_id].append(-1)
        busy_cycles[host_id].append(-1)
        total_cycles[host_id].append(0)
        stall_cycles_when_thread_ready[host_id].append([])
        stall_cycles_when_thread_ready_avg[host_id].append(0)

        for hart_id in range(hart):
            stall_cycles_when_thread_ready[host_id][core_id].append(-1)

file = open(statFile, "r")
csvFile = csv.reader(file)
# skip header
next(csvFile)

# ComponentName, StatisticName, StatisticSubId, StatisticType, SimTime, Rank, Sum, SumSQ, Count, Min, Max
for line in csvFile:
    component = line[0]
    component_split = component.split("_")
    statistic = line[1]
    statisticSubId = line[2]
    statisticSubId_split = statisticSubId.split("_")
    value = int(line[6])

    if statistic == phase + "_stall_cycles":
        if component_split[0] == "command":
            continue
        else:
            core_id = int(component_split[1])
            host_id = int(component_split[3][3:])
            stall_cycles[host_id][core_id] = value
            total_cycles[host_id][core_id] += value
    elif statistic == phase + "_stall_cycles_when_ready":
        if component_split[0] == "command":
            continue
        else:
            core_id = int(component_split[1])
            host_id = int(component_split[3][3:])
            thread_id = int(statisticSubId_split[1])
            stall_cycles_when_thread_ready[host_id][core_id][thread_id] = value
            stall_cycles_when_thread_ready_avg[host_id][core_id] += value/hart
    elif statistic == phase + "_busy_cycles":
        if component_split[0] == "command":
            continue
        else:
            core_id = int(component_split[1])
            host_id = int(component_split[3][3:])
            busy_cycles[host_id][core_id] = value
            total_cycles[host_id][core_id] += value

print(stall_cycles)
print(total_cycles)
print(stall_cycles_when_thread_ready_avg)
