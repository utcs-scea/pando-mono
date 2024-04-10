# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

import sys

if len(sys.argv) != 2:
    print("Usage: python3 projector.py (number of pxns)")
    sys.exit(1)

numHosts = int(sys.argv[1])

localMirrorMaps = []
localMasterMaps = []

for i in range(numHosts):
    localMirrorMaps.append({})
    localMasterMaps.append([{} for j in range(numHosts)])

for line in sys.stdin:
    line_split = line.strip().split()
    if len(line_split) != 9:
        continue

    if line_split[0] == "(Mirror)":
        hostID = int(line_split[2])
        mirrorTopologyID = line_split[4]
        masterTopologyID = line_split[6]
        masterHost = int(line_split[8])
        localMirrorMaps[hostID][mirrorTopologyID] = [masterTopologyID, masterHost]
    elif line_split[0] == "(Master)":
        hostID = int(line_split[2])
        fromHost = int(line_split[4])
        masterTopologyID = line_split[6]
        mirrorTopologyID = line_split[8]
        localMasterMaps[hostID][fromHost][masterTopologyID] = mirrorTopologyID

for hostID in range(numHosts):
    for mirrorTopologyID, mirrorInfo in localMirrorMaps[hostID].items():
        masterTopologyID = mirrorInfo[0]
        masterHost = mirrorInfo[1]

        temp = localMasterMaps[masterHost][hostID][masterTopologyID]

        if (temp != mirrorTopologyID):
            sys.exit(1)

print("PASS")
