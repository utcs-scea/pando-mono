# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

import sys


vert = []
tot_verts = int(sys.argv[1])
for i in range(0,tot_verts):
  vert.append({})

for line in sys.stdin:
  src, dst = line.split()
  src, dst = int(src), int(dst)
  if src >= tot_verts or dst >= tot_verts:
    continue
  if src > dst:
    src, dst = dst, src
  vert[src][dst] = True;

for d in vert:
  for j in sorted(d.keys()):
    print(j, end=' ')
  print()
