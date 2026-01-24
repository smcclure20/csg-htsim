#!/usr/bin/env python

# Permutation matrix with only interpod flows. ( symmetric ring)

import os
import sys
from random import seed, shuffle
#print(sys.argv)
if len(sys.argv) != 6:
    print("Usage: python3 gen_permutation_ring.py <filename> <nodes> <offset> <flowsize> <extrastarttime>")
    sys.exit()
filename = sys.argv[1]
nodes = int(sys.argv[2])
conns = nodes
offset = int(sys.argv[3])
flowsize = int(sys.argv[4])
extrastarttime = float(sys.argv[5])

print("Nodes: ", nodes)
print("Offset: ", offset)
print("Flowsize: ", flowsize, "bytes")
print("ExtraStartTime: ", extrastarttime, "us")

f = open(filename, "w")
print("Nodes", nodes, file=f)
print("Connections", conns, file=f)


all_srcs = []
all_dsts = []
srcs = []
dsts = []
for n in range(nodes):
    srcs.append(n)
    dst = n + offset
    if dst >= nodes:
        dst = ((dst % nodes) + 1) % offset
    dsts.append(dst)
all_srcs.extend(srcs)
all_dsts.extend(dsts)



#print(dsts)

for n in range(conns):
    out = str(all_srcs[n]) + "->" + str(all_dsts[n]) + " id " + str(n+1) + " start " + str(int(extrastarttime * 1000000)) + " size " + str(flowsize)
    print(out, file=f)

f.close()
