#!/bin/bash

# TCP Results in default

Trim
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/f9/trim/opt-ata-128n-2MB-trim-f9-spray-${i}.csv -ratecoef 0.9 -trim -fails 9 -failpct 0.8
done

# Trim + RTS
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/f9/trim-rts/opt-ata-128n-2MB-trimrts-f9-spray-${i}.csv -ratecoef 0.9 -trim -rts -fails 9 -failpct 0.8
done

# # Coding
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/f9/coding/opt-ata-128n-2MB-coding-f9-spray-${i}.csv -ratecoef 0.9 -fails 9 -failpct 0.8
# done
