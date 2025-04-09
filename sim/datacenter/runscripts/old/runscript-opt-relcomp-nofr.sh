#!/bin/bash

# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-nofr-trimrts-spray-flows.csv -ratecoef 0.9 -trim -rts -nofr
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-nofr-trim-spray-flows.csv -ratecoef 0.9 -trim -nofr

./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-backoff-trimrts-spray-flows.csv -ratecoef 0.9 -trim -rts
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-backoff-trim-spray-flows.csv -ratecoef 0.9 -trim


# # Then compare with failure too
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-f2-trimrts-spray-flows.csv -ratecoef 0.9 -trim -rts -fails 2
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-f2-trim-spray-flows.csv -ratecoef 0.9 -trim -fails 2
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-f2-spray-flows.csv -ratecoef 0.9 -fails 2
# ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-erase-f2-spray-flows.csv -ratecoef 0.9 -fails 2

# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-f9-trimrts-spray-flows.csv -ratecoef 0.9 -trim -rts -fails 9 -failpct 0.8
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-f9-trim-spray-flows.csv -ratecoef 0.9 -trim -fails 9 -failpct 0.8
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-f9-spray-flows.csv -ratecoef 0.9 -fails 9 -failpct 0.8
# ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-ata-128n-2MB-erase-f9-spray-flows.csv -ratecoef 0.9 -fails 9 -failpct 0.8
