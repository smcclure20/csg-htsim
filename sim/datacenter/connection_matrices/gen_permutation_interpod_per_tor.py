#!/usr/bin/env python

# Creates one internal flow per tor. All others are interpod
import os
import sys
from random import seed, shuffle
#print(sys.argv)
if len(sys.argv) != 6:
    print("Usage: python3 gen_permutation_interpod_per_tor.py <filename> <nodes> <tor_size> <flowsize> <extrastarttime>")
    sys.exit()
filename = sys.argv[1]
nodes = int(sys.argv[2])
conns = nodes
tor_size = int(sys.argv[3])
flowsize = int(sys.argv[4])
extrastarttime = float(sys.argv[5])

print("Nodes: ", nodes)
print("Nodes per ToR: ", tor_size)
print("Nodes per Pod: ", tor_size * tor_size)
pod_size = tor_size * tor_size
num_pods = tor_size * 2
tors_per_pod = tor_size
print("Flowsize: ", flowsize, "bytes")
print("ExtraStartTime: ", extrastarttime, "us")

f = open(filename, "w")
print("Nodes", nodes, file=f)
print("Connections", conns, file=f)


srcs = []
dsts = []
for n in range(nodes):
    srcs.append(n)
    if (n % tor_size) != 0: # All but first node under tor
        target_pod = ((n // pod_size) + 1) % num_pods # Shift over a pod
        dst = target_pod * pod_size + (n % pod_size)
        if (n% tor_size) == 1:
            dst -= 1 # send to first node in tor rather than coutnerpart
    else:
        dst = n + 1
    dsts.append(dst)



#print(dsts)

for n in range(conns):
    out = str(srcs[n]) + "->" + str(dsts[n]) + " id " + str(n+1) + " start " + str(int(extrastarttime * 1000000)) + " size " + str(flowsize)
    print(out, file=f)

f.close()
