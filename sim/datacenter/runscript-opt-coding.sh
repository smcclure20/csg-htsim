#!/bin/bash

# Base case
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-latest-3-13/opt-ata-128n-2MB-ecmp-flows.csv 
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-latest-3-13/opt-ata-128n-2MB-spray-flows.csv 
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-latest-3-13/opt-ata-128n-2MB-plbecn4-flows.csv 
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-latest-3-13/opt-ata-128n-2MB-rr-flows.csv 

# ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-latest-3-13/optcoding-ata-128n-2MB-ecmp-flows.csv 
./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-latest-3-13/optcoding-ata-128n-2MB-spray-flows.csv 
./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-latest-3-13/optcoding-ata-128n-2MB-plbecn4-flows.csv 
# ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-latest-3-13/optcoding-ata-128n-2MB-rr-flows.csv
