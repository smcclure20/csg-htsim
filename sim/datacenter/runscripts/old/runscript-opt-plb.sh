#!/bin/bash

./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -of ../../results/opt/opt-ata-128n-2MB-12mrtt-plb-flows.csv
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -of ../../results/opt/opt-ata-128n-2MB-f2-12mrtt-plb-flows.csv -fails 2

