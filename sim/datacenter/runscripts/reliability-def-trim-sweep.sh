#!/bin/bash

rates=(0.7)


for rate in ${rates[@]}
do
    # Trim
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/trimmingsweep/${rate}/trim/opt-ata-128n-2MB-p${rate}-nofr-trim-spray-${i}.csv -ratecoef ${rate}  -trim -nofr
    done

    # Trim + RTS
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/trimmingsweep/${rate}/trim-rts/opt-ata-128n-2MB-p${rate}-nofr-trimrts-spray-${i}.csv -ratecoef ${rate} -trim -rts -nofr
    done
done