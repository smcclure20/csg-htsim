#!/bin/bash
conns=(128 1280 2560 5120 10240 12800)

for conn in ${conns[@]}
do 
    matrix="perm_128n_${conn}c_s1_2MB.cm"
    echo "Running simulations for connection matrix: ${matrix} with ${conn} connections"
    # Trim
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/def/trim-ced/opt-perm${conn}-128n-2MB-trim-spray-${i}.csv -ratecoef 0.9 -trimced
    done

    # Trim + RTS
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/def/trim-rts-ced/opt-perm${conn}-128n-2MB-trimrts-spray-${i}.csv -ratecoef 0.9 -trimced -rts
    done

done
