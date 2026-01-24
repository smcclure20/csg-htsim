#!/bin/bash

file = open("perm_128n_128c_s1_2MB.cm", "r")
k = 8
podsize = (k/2)**2
interpods = []
intrapods = []

for line in file.readlines():
    if line.startswith("Nodes") or line.startswith("Connections"):
        continue
    flow = line.split(" ")[0]
    start = int(flow.split("->")[0])
    end = int(flow.split("->")[1])
    if (start // podsize) == (end // podsize):
        intrapods.append(start)
    else:
        interpods.append(start)

print("Interpod flows: ", len(interpods))
print("Intrapod flows: ", len(intrapods))

inter_count_per_pod = {}
for node in interpods:
    pod = node // podsize
    count = inter_count_per_pod.get(pod, 0)
    inter_count_per_pod[pod] = count + 1

print(inter_count_per_pod)