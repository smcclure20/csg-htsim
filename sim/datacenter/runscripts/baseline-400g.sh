#!/bin/bash

# ECMP
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-4-2/400g/ecmp/opt-ata-128n-2MB-400g-ecmp-${i}.csv -ratecoef 0.9 -linkspeed 400000
done

# PLB
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-4-2/400g/plb/opt-ata-128n-2MB-400g-plb4-${i}.csv -ratecoef 0.9 -linkspeed 400000
done

# RR
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-4-2/400g/rr/opt-ata-128n-2MB-400g-rr-${i}.csv -ratecoef 0.9 -linkspeed 400000
done

# Host spray
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/400g/spray/opt-ata-128n-2MB-400g-spray-${i}.csv -ratecoef 0.9 -linkspeed 400000
done

# TODO: Flowlet and adaptive versions
