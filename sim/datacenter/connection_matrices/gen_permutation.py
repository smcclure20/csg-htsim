#!/usr/bin/env python

# Generate a permutation traffic matrix.
# python gen_pemutation.py <nodes> <conns> <flowsize> <extrastarttime>
# Parameters:
# <nodes>   number of nodes in the topology
# <conns>    number of active connections
# <flowsize>   size of the flows in bytes
# <extrastarttime>   How long in microseconds to space the start times over (start time will be random in between 0 and this time).  Can be a float.
# <randseed>   Seed for random number generator, or set to 0 for random seed

import os
import sys
from random import seed, shuffle
#print(sys.argv)
if len(sys.argv) != 7:
    print("Usage: python3 gen_permutation.py <filename> <nodes> <conns> <flowsize> <extrastarttime> <randseed>")
    sys.exit()
filename = sys.argv[1]
nodes = int(sys.argv[2])
conns = int(sys.argv[3])
flowsize = int(sys.argv[4])
extrastarttime = float(sys.argv[5])
randseed = int(sys.argv[6])

print("Nodes: ", nodes)
print("Connections: ", conns)
print("Flowsize: ", flowsize, "bytes")
print("ExtraStartTime: ", extrastarttime, "us")
print("Random Seed ", randseed)

f = open(filename, "w")
print("Nodes", nodes, file=f)
print("Connections", conns, file=f)

if randseed != 0:
    seed(randseed)

all_srcs = []
all_dsts = []
for c in range(int(conns/nodes)):
    srcs = []
    dsts = []
    for n in range(nodes):
        srcs.append(n)
        dsts.append(n)
    shuffle(srcs)
    shuffle(dsts)   
    all_srcs.extend(srcs)
    all_dsts.extend(dsts)


#print(srcs)
#print(dsts)

# eliminate any duplicates - a node should not send to itself
for c in range(int(conns/nodes)):
    for n in range(nodes):
        if all_srcs[n + nodes*c] == all_dsts[n+ nodes*c]:
            i = (n+1)%nodes
            #print("swapping", n, "and", i)
            tmp = all_dsts[n+ nodes*c]
            all_dsts[n+ nodes*c] = all_dsts[i+ nodes*c]
            all_dsts[i+ nodes*c] = tmp
            if all_srcs[n + nodes*c] == all_dsts[n + nodes*c]:
                print("Error: still found a self-loop after swapping! This should not happen.")
                sys.exit(1)

#print(dsts)

for n in range(conns):
    out = str(all_srcs[n]) + "->" + str(all_dsts[n]) + " id " + str(n+1) + " start " + str(int(extrastarttime * 1000000)) + " size " + str(flowsize)
    print(out, file=f)

f.close()
