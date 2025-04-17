#!/bin/bash

# TCP Results in default
# CES stands for COMPOSITE_ECN + small buffer
# Trim
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default/trim-ced/opt-ata-128n-2MB-trim-spray-${i}.csv -ratecoef 0.9 -trimced
done

# Trim + RTS
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default/trim-rts-ced/opt-ata-128n-2MB-trimrts-spray-${i}.csv -ratecoef 0.9 -trimced -rts 
    
done

# Trim
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default/trim/opt-ata-128n-2MB-trim-spray-${i}.csv -ratecoef 0.9 -trim
done

# Trim + RTS
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default/trim-rts/opt-ata-128n-2MB-trimrts-spray-${i}.csv -ratecoef 0.9 -trim -rts 
    
done
