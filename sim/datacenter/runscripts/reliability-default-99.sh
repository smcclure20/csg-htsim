#!/bin/bash

# # TCP
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default99/tcp/opt-ata-128n-2MB-99-tcp-spray-${i}.csv -ratecoef 0.99 -trimcomp
# done

# # Trim
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default99/trim/opt-ata-128n-2MB-99-trim-spray-${i}.csv -ratecoef 0.99 -trim
# done

# Trim + RTS
for i in {8..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default99/trim-rts/opt-ata-128n-2MB-99-trimrts-spray-${i}.csv -ratecoef 0.99 -trim -rts
done

# Coding
for i in {1..10}
do
    ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default99/coding/opt-ata-128n-2MB-99-coding-spray-${i}.csv -ratecoef 0.99 -k 25
done

