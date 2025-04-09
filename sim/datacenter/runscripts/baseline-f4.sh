#!/bin/bash

# ECMP
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-4-2/f4/ecmp/opt-ata-128n-2MB-f4-ecmp-${i}.csv -ratecoef 0.9 -fails 4 -failpct 0.6
done

# PLB
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-4-2/f4/plb/opt-ata-128n-2MB-f4-plb4-${i}.csv -ratecoef 0.9 -fails 4 -failpct 0.6
done

# RR
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-4-2/f4/rr/opt-ata-128n-2MB-f4-rr-${i}.csv -ratecoef 0.9 -fails 4 -failpct 0.6
done

# Host spray
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/f4/spray/opt-ata-128n-2MB-f4-spray-${i}.csv -ratecoef 0.9 -fails 4 -failpct 0.6
done

# TODO: Flowlet and adaptive versions
