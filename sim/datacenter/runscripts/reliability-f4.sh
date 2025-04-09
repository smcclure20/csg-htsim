#!/bin/bash

# TCP Results in default

Trim
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/f4/trim/opt-ata-128n-2MB-trim-f4-spray-${i}.csv -ratecoef 0.9 -trim -fails 4 -failpct 0.6
done

# Trim + RTS
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/f4/trim-rts/opt-ata-128n-2MB-trimrts-f4-spray-${i}.csv -ratecoef 0.9 -trim -rts -fails 4 -failpct 0.6
done

# # Coding
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/f4/coding/opt-ata-128n-2MB-coding-f4-spray-${i}.csv -ratecoef 0.9 -fails 4 -failpct 0.6
# done
