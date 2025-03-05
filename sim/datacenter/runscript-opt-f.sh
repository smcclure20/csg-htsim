#!/bin/bash

# ./htsim_constcca -end 260000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt2/opt-ata-128n-2MB-f2-ecmp-flows.csv -fails 2
# ./htsim_constcca -end 260000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt2/opt-ata-128n-2MB-f2-spray-flows.csv -fails 2
# # This one has incomplete flows (below)
# ./htsim_constcca -end 360000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -of ../../results/opt2/opt-ata-128n-2MB-f2-15mrtt-plb-flows.csv -fails 2
# ./htsim_constcca -end 260000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt2/opt-ata-128n-2MB-f2-rr-flows.csv -fails 2

./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt/opt-ata-128n-2MB-f4-ecmp-flows.csv -fails 4 -failpct 0.6
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt/opt-ata-128n-2MB-f4-spray-flows.csv -fails 4 -failpct 0.6
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -of ../../results/opt/opt-ata-128n-2MB-f4-15mrtt-plb-flows.csv -fails 4 -failpct 0.6
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt/opt-ata-128n-2MB-f4-rr-flows.csv -fails 4 -failpct 0.6

