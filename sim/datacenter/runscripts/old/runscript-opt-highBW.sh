#!/bin/bash

./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-400g-nofr-trimrts-spray-flows.csv -ratecoef 0.9 -trim -rts -linkspeed 400000 -nofr
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-400g-nofr-trim-spray-flows.csv -ratecoef 0.9 -trim -linkspeed 400000 -nofr
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-400g-spray-flows.csv -ratecoef 0.9 -linkspeed 400000
# ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-400g-erase-spray-flows.csv -ratecoef 0.9 -linkspeed 400000

