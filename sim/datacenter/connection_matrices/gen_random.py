#!/usr/bin/env python

# Generate a random traffic matrix.
# python gen_ranom.py <nodes> <conns> <flowsize> <extrastarttime>
# Parameters:
# <nodes>   number of nodes in the topology
# <conns>    number of active connections
# <flowsize>   size of the flows in bytes TODO: make this a distribution
# <extrastarttime>   How long in microseconds to space the start times over (start time will be random in between 0 and this time).  Can be a float.
# <randseed>   Seed for random number generator, or set to 0 for random seed

import os
import sys
from random import seed, shuffle, choice, uniform
print(sys.argv)
if len(sys.argv) != 7:
    print("Usage: python gen_random.py <filename> <nodes> <conns> <flowsize> <extrastarttime> <randseed>")
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

srcs = []
dsts = []
if randseed != 0:
    seed(randseed)

for c in range(conns):
    src = choice(list(range(nodes)))
    dst = src
    while dst == src:
        dst = choice(list(range(nodes)))
    srcs.append(src)
    dsts.append(dst)
    
for n in range(conns):
    starttime = uniform(0, extrastarttime)
    out = str(srcs[n]) + "->" + str(dsts[n]) + " id " + str(n+1) + " start " + str(int(starttime * 1000000)) + " size " + str(flowsize)
    print(out, file=f)

f.close()