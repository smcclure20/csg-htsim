#!/bin/bash

# Trim
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default/trim-nofr/opt-ata-128n-2MB-nofr-trim-spray-${i}.csv -ratecoef 0.9 -trim -nofr
done

# Trim + RTS
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default/trim-rts-nofr/opt-ata-128n-2MB-nofr-trimrts-spray-${i}.csv -ratecoef 0.9 -trim -rts -nofr
done


# Trim
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default/trim-0.8/opt-ata-128n-2MB-p0.8-trim-spray-${i}.csv -ratecoef 0.8 -trim
done

# Trim + RTS
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default/trim-rts-0.8/opt-ata-128n-2MB-p0.8-trimrts-spray-${i}.csv -ratecoef 0.8 -trim -rts
done

