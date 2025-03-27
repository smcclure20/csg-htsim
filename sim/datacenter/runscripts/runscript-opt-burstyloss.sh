#!/bin/bash

# Base case
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-flaky2-trimrts-spray-flows.csv -ratecoef 0.9 -flakylinks 2 -trim -rts
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-flaky2-trim-spray-flows.csv -ratecoef 0.9 -flakylinks 2 -trim
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-flaky2-spray-flows.csv -ratecoef 0.9 -flakylinks 2
# ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-flaky2-erase-spray-flows.csv -ratecoef 0.9 -flakylinks 2

# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-flaky2-oldq-trimrts-spray-flows.csv -ratecoef 0.9 -flakylinks 2 -trim -rts
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-flaky2-oldq-trim-spray-flows.csv -ratecoef 0.9 -flakylinks 2 -trim

# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-flaky9-trimrts-spray-flows.csv -ratecoef 0.9 -flakylinks 9 -trim -rts
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-flaky9-trim-spray-flows.csv -ratecoef 0.9 -flakylinks 9 -trim
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-flaky9-spray-flows.csv -ratecoef 0.9 -flakylinks 9
# ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-flaky9-erase-spray-flows.csv -ratecoef 0.9 -flakylinks 9

./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-3-25/opt-ata-128n-2MB-flaky2-trimrts-ecmp-flows.csv -ratecoef 0.9 -flakylinks 2 -trim -rts
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp  -of ../../results/opt-3-25/opt-ata-128n-2MB-flaky2-trim-ecmp-flows.csv -ratecoef 0.9 -flakylinks 2 -trim
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp  -of ../../results/opt-3-25/opt-ata-128n-2MB-flaky2-ecmp-flows.csv -ratecoef 0.9 -flakylinks 2
./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp  -of ../../results/opt-3-25/opt-ata-128n-2MB-flaky2-erase-ecmp-flows.csv -ratecoef 0.9 -flakylinks 2


./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-3-25/opt-ata-128n-2MB-flaky9-trimrts-ecmp-flows.csv -ratecoef 0.9 -flakylinks 9 -trim -rts
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp  -of ../../results/opt-3-25/opt-ata-128n-2MB-flaky9-trim-ecmp-flows.csv -ratecoef 0.9 -flakylinks 9 -trim
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp  -of ../../results/opt-3-25/opt-ata-128n-2MB-flaky9-ecmp-flows.csv -ratecoef 0.9 -flakylinks 9
./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp  -of ../../results/opt-3-25/opt-ata-128n-2MB-flaky9-erase-ecmp-flows.csv -ratecoef 0.9 -flakylinks 9

