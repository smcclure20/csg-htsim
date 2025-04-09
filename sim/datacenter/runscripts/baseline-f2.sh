#!/bin/bash

# ECMP
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-4-2/f2/ecmp/opt-ata-128n-2MB-f2-ecmp-${i}.csv -ratecoef 0.9 -fails 2
done

# PLB
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-4-2/f2/plb/opt-ata-128n-2MB-f2-plb4-${i}.csv -ratecoef 0.9 -fails 2
done

# RR
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-4-2/f2/rr/opt-ata-128n-2MB-f2-rr-${i}.csv -ratecoef 0.9 -fails 2
done

# Host spray
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/f2/spray/opt-ata-128n-2MB-f2-spray-${i}.csv -ratecoef 0.9 -fails 2
done

# TODO: Flowlet and adaptive versions
