#!/bin/bash

# Trim
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default/trim-cel/opt-ata-128n-2MB-trim-spray-${i}.csv -ratecoef 0.9 -trimcel
done

# Trim + RTS
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/default/trim-rts-cel/opt-ata-128n-2MB-trimrts-spray-${i}.csv -ratecoef 0.9 -trimcel -rts
done

conns=(128 1280 2560 5120 10240 12800)

for conn in ${conns[@]}
do 
    matrix="perm_128n_${conn}c_s1_2MB.cm"
    echo "Running simulations for connection matrix: ${matrix} with ${conn} connections"
    # Trim
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/def/trim-cel/opt-perm${conn}-128n-2MB-trim-spray-${i}.csv -ratecoef 0.9 -trimcel
    done

    # Trim + RTS
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/def/trim-rts-cel/opt-perm${conn}-128n-2MB-trimrts-spray-${i}.csv -ratecoef 0.9 -trimcel -rts
    done

done
