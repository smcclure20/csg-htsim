#!/bin/bash

# Failures
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-latest-3-13/opt-ata-128n-2MB-f2-ecmp-flows.csv -fails 2
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-latest-3-13/opt-ata-128n-2MB-f2-spray-flows.csv -fails 2
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-latest-3-13/opt-ata-128n-2MB-f2-plbecn4-flows.csv -fails 2 
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-latest-3-13/opt-ata-128n-2MB-f2-rr-flows.csv -fails 2

# ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-latest-3-13/optcoding-ata-128n-2MB-f2-ecmp-flows.csv -fails 2
./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-latest-3-13/optcoding-ata-128n-2MB-f2-spray-flows.csv -fails 2
./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-latest-3-13/optcoding-ata-128n-2MB-f2-plbecn4-flows.csv -fails 2
# ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-latest-3-13/optcoding-ata-128n-2MB-f2-rr-flows.csv -fails 2

# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-latest-3-13/opt-ata-128n-2MB-f9-ecmp-flows.csv -fails 9 -failpct 0.8
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-latest-3-13/opt-ata-128n-2MB-f9-spray-flows.csv -fails 9 -failpct 0.8
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-latest-3-13/opt-ata-128n-2MB-f9-plbecn4-flows.csv -fails 9 -failpct 0.8 
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-latest-3-13/opt-ata-128n-2MB-f9-rr-flows.csv -fails 9 -failpct 0.8

# ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-latest-3-13/optcoding-ata-128n-2MB-f9-ecmp-flows.csv -fails 9 -failpct 0.8
./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-latest-3-13/optcoding-ata-128n-2MB-f9-spray-flows.csv -fails 9 -failpct 0.8
./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-latest-3-13/optcoding-ata-128n-2MB-f9-plbecn4-flows.csv -fails 9 -failpct 0.8
# ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-latest-3-13/optcoding-ata-128n-2MB-f9-rr-flows.csv -fails 9 -failpct 0.8