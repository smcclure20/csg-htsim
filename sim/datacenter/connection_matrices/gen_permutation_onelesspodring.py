#!/usr/bin/env python

# Permutation matrix with all interpod flows except 1 per pod

import os
import sys
from random import seed, shuffle
#print(sys.argv)
if len(sys.argv) != 6:
    print("Usage: python3 gen_permutation_ring.py <filename> <nodes> <podsize> <flowsize> <extrastarttime>")
    sys.exit()
filename = sys.argv[1]
nodes = int(sys.argv[2])
conns = nodes
podsize = int(sys.argv[3])
flowsize = int(sys.argv[4])
extrastarttime = float(sys.argv[5])

print("Nodes: ", nodes)
print("podsize: ", podsize)
print("Flowsize: ", flowsize, "bytes")
print("ExtraStartTime: ", extrastarttime, "us")

f = open(filename, "w")
print("Nodes", nodes, file=f)
print("Connections", conns, file=f)


srcs = []
dsts = []
for n in range(nodes):
    srcs.append(n)
    if n % podsize != 0:
        dst = n + podsize if n + podsize < nodes else ((dst % nodes) + 1) % podsize
        if dst % podsize == 1:
            dst -= 1
    else:
        dst = n + 1
    dsts.append(dst)



#print(dsts)

for n in range(conns):
    out = str(srcs[n]) + "->" + str(dsts[n]) + " id " + str(n+1) + " start " + str(int(extrastarttime * 1000000)) + " size " + str(flowsize)
    print(out, file=f)

f.close()
