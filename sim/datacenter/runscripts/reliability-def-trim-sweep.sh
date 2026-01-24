#!/bin/bash

rates=(1.0)


for rate in ${rates[@]}
do
    # Trim
    for i in {4..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/trimmingsweep/${rate}/trim/opt-ata-128n-2MB-p${rate}-nofr-trim-spray-${i}.csv -ratecoef ${rate}  -trim -nofr
    done
done