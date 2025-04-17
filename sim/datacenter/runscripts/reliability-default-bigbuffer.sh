#!/bin/bash

# # TCP
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/bigbuffer/tcp/opt-ata-128n-2MB-buf100-tcp-spray-${i}.csv -ratecoef 0.9 -q 100
# done

# # Trim
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/bigbuffer/trim/opt-ata-128n-2MB-buf100-trim-spray-${i}.csv -ratecoef 0.9 -trim -q 100
# done

# # Trim + RTS
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/bigbuffer/trim-rts/opt-ata-128n-2MB-buf100-trimrts-spray-${i}.csv -ratecoef 0.9 -trim -rts -q 100
# done

# # Coding
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/bigbuffer/coding/opt-ata-128n-2MB-buf100-coding-spray-${i}.csv -ratecoef 0.9 -k 25 -q 100
# done

# ROCE
for i in {1..10}
do
    ./htsim_roce_new -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/bigbuffer/roce0.9/opt-ata-128n-2MB-buf100-roce0.9-spray-${i}.csv -ratecoef 0.9 -q 100
done
