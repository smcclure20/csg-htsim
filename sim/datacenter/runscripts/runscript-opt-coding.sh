#!/bin/bash

# Base case
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-latest-3-13/opt-ata-128n-2MB-sloss0001-ecmp-flows.csv -sloss 0.0001
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-latest-3-13/opt-ata-128n-2MB-sloss0001-spray-flows.csv -sloss 0.0001
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-latest-3-13/opt-ata-128n-2MB-sloss0001-plbecn4-flows.csv -sloss 0.0001
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-latest-3-13/opt-ata-128n-2MB-sloss0001-rr-flows.csv -sloss 0.0001

./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-latest-3-13/optcoding-ata-128n-2MB-sloss0001-ecmp-flows.csv -sloss 0.0001
./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-latest-3-13/optcoding-ata-128n-2MB-sloss0001-spray-flows.csv -sloss 0.0001
./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-latest-3-13/optcoding-ata-128n-2MB-sloss0001-plbecn4-flows.csv -sloss 0.0001
./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-latest-3-13/optcoding-ata-128n-2MB-sloss0001-rr-flows.csv -sloss 0.0001
