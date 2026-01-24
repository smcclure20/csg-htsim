#!/bin/bash

# # TCP
# for i in {7..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability-optrate/bigbuffer/tcp/opt-ata-128n-2MB-buf100-tcp-spray-${i}.csv -ratecoef 1 -q 100
# done

# # Trim
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability-optrate/bigbuffer/trim/opt-ata-128n-2MB-buf100-trim_nofr-spray-${i}.csv -ratecoef 1 -trim -nofr -q 100
# done

# # Trim + RTS
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability-optrate/bigbuffer/trimrts/opt-ata-128n-2MB-buf100-trimrts_nofr-spray-${i}.csv -ratecoef 1 -trim -rts -nofr -q 100
# done

# Coding
for i in {1..10}
do
    ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability-optrate/bigbuffer/coding5/opt-ata-128n-2MB-buf100-coding-spray-${i}.csv -ratecoef 1 -k 25 -q 100
done

# # ROCE
# for i in {1..10}
# do
#     ./htsim_roce_new -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability-optrate/bigbuffer/roce/opt-ata-128n-2MB-buf100-roce-spray-${i}.csv -ratecoef 1 -q 100
# done
