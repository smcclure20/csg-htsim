#!/bin/bash

# TCP Results in default

# Trim
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/f4/trim-nofr/opt-ata-128n-2MB-trim_nofr-f4-spray-${i}.csv -ratecoef 0.9 -trim -nofr -fails 4 -failpct 0.6
done

# Trim + RTS
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/f4/trim-rts-nofr/opt-ata-128n-2MB-trimrts_nofr-f4-spray-${i}.csv -ratecoef 0.9 -trim -rts -nofr -fails 4 -failpct 0.6
done

# # Coding
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/f4/coding/opt-ata-128n-2MB-coding-f4-spray-${i}.csv -ratecoef 0.9 -fails 4 -failpct 0.6 -k 25
# done

# # ROCE
# for i in {1..10}
# do
#     ./htsim_roce_new -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/f4/roce0.9/opt-ata-128n-2MB-roce0.9-f4-spray-${i}.csv -ratecoef 0.9 -fails 4 -failpct 0.6
# done

