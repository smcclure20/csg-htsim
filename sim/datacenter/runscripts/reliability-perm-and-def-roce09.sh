#!/bin/bash

# ROCE
for i in {1..10}
do
    ./htsim_roce_new -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default/roce0.9/opt-ata-128n-2MB-roce09-spray-${i}.csv -ratecoef 0.9
done

conns=(128 1280 2560 5120 10240 12800)

for conn in ${conns[@]}
do 
    matrix="perm_128n_${conn}c_s1_2MB.cm"
    echo "Running simulations for connection matrix: ${matrix} with ${conn} connections"
    # Roce
    for i in {1..10}
    do
        ./htsim_roce_new -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/def/roce0.9/opt-perm${conn}-128n-2MB-trim-spray-${i}.csv -ratecoef 0.9
    done


done
