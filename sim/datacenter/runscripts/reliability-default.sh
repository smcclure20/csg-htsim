#!/bin/bash

# TCP Results in default

# # Trim
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default/trim/opt-ata-128n-2MB-trim-spray-${i}.csv -ratecoef 0.9 -trim
# done

# # Trim + RTS
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default/trim-rts/opt-ata-128n-2MB-trimrts-spray-${i}.csv -ratecoef 0.9 -trim -rts
# done

# # Coding
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default/coding/opt-ata-128n-2MB-coding-spray-${i}.csv -ratecoef 0.9 -k 25
# done

# # ROCE
# for i in {1..10}
# do
#     ./htsim_roce_new -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default/roce/opt-ata-128n-2MB-roce-spray-${i}.csv -ratecoef 1
# done
