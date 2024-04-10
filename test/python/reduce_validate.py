# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

import sys

if len(sys.argv) != 2:
    print("Usage: python3 projector.py (number of pxns)")
    sys.exit(1)

numHosts = int(sys.argv[1])

localMirrors = []
localMasters = []

for i in range(numHosts):
    localMirrors.append({})
    localMasters.append({})

for line in sys.stdin:
    line_split = line.strip().split()
    if len(line_split) < 9:
        continue

    if line_split[0] == "(Mirror)":
        hostID = int(line_split[2])
        mirrorTopologyID = line_split[4]
        masterTopologyID = line_split[6]
        masterHost = int(line_split[8])
        localMirrors[hostID][mirrorTopologyID] = [masterTopologyID, masterHost]
    elif line_split[0] == "(Master)":
        hostID = int(line_split[2])
        masterTopologyID = line_split[4]
        masterTokenID = int(line_split[6])
        masterData = int(line_split[8])
        masterBit = int(line_split[10])
        localMasters[hostID][masterTopologyID] = [masterTokenID, masterData, masterBit]

for hostID in range(numHosts):
    for mirrorTopologyID, mirrorInfo in localMirrors[hostID].items():
        masterTopologyID = mirrorInfo[0]
        masterHost = mirrorInfo[1]

        masterInfo = localMasters[masterHost][masterTopologyID]
        masterTokenID = masterInfo[0]
        masterData = masterInfo[1]
        masterBit = masterInfo[2]

        if (masterBit != 1 or masterData != masterTokenID+1):
            sys.exit(1)

print("PASS")
