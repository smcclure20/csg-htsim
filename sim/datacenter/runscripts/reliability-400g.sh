#!/bin/bash

# TCP Results in default

# # Trim
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/400g/trim/opt-ata-128n-2MB-trim-400g-spray-${i}.csv -ratecoef 0.9 -trim -linkspeed 400000
# done

# # Trim + RTS
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/400g/trim-rts/opt-ata-128n-2MB-trimrts-400g-spray-${i}.csv -ratecoef 0.9 -trim -rts -linkspeed 400000
# done

# # Coding
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/400g/coding/opt-ata-128n-2MB-coding-400g-spray-${i}.csv -ratecoef 0.9 -linkspeed 400000
# done

# ROCE
for i in {1..10}
do
    ./htsim_roce_new -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/400g/roce/opt-ata-128n-2MB-roce-spray-${i}.csv -ratecoef 1 -linkspeed 400000
done
