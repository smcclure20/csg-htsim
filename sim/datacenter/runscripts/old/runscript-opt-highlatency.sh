#!/bin/bash

./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-19/opt-ata-128n-2MB-lat5-trimrts-spray-flows.csv -ratecoef 0.9 -trim -rts -lat 5
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-19/opt-ata-128n-2MB-lat5-trim-spray-flows.csv -ratecoef 0.9 -trim -lat 5
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-19/opt-ata-128n-2MB-lat5-spray-flows.csv -ratecoef 0.9 -lat 5
./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-19/opt-ata-128n-2MB-lat5-erase-spray-flows.csv -ratecoef 0.9 -lat 5

