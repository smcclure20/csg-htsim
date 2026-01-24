#!/bin/bash

# # TCP
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default99/tcp/opt-ata-128n-2MB-99-tcp-spray-${i}.csv -ratecoef 0.99 -trimcomp
# done

# Trim
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default99/trim-nofr/opt-ata-128n-2MB-99-trimnofr-spray-${i}.csv -ratecoef 0.99 -trim -nofr
done

# Trim + RTS
for i in {8..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default99/trim-rts-nofr/opt-ata-128n-2MB-99-trimrtsnofr-spray-${i}.csv -ratecoef 0.99 -trim -rts -nofr
done

# # Coding
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default99/coding/opt-ata-128n-2MB-99-coding-spray-${i}.csv -ratecoef 0.99 -k 25
# done

