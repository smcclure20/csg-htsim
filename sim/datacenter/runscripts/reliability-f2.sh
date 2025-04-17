#!/bin/bash

# TCP Results in default

# Trim
for i in {1..7}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/f2/trim-0.75-nofr/opt-ata-128n-2MB-trim0.75_nofr-f2-spray-${i}.csv -ratecoef 0.75 -trim -nofr -fails 2
done

# # Trim + RTS
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/f2/trim-rts-0.75-nofr/opt-ata-128n-2MB-trimrts0.75_nofr-f2-spray-${i}.csv -ratecoef 0.75 -trim -rts -nofr -fails 2
# done

# # Coding
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/f2/coding/opt-ata-128n-2MB-coding-f2-spray-${i}.csv -ratecoef 0.9 -fails 2 -k 25
# done

# # ROCE
# for i in {1..10}
# do
#     ./htsim_roce_new -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/f2/roce/opt-ata-128n-2MB-roce-f2-spray-${i}.csv -ratecoef 1 -fails 2
# done
