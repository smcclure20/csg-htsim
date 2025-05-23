#!/bin/bash

./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-19/opt-ata-128n-2MB-q1000-trimrts-spray-flows.csv -ratecoef 0.9 -trim -rts -q 1000
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-19/opt-ata-128n-2MB-q1000-trim-spray-flows.csv -ratecoef 0.9 -trim -q 1000
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-19/opt-ata-128n-2MB-q1000-spray-flows.csv -ratecoef 0.9 -q 1000
./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-19/opt-ata-128n-2MB-q1000-erase-spray-flows.csv -ratecoef 0.9 -q 1000

