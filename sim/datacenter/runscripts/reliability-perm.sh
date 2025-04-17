#!/bin/bash
conns=(128 1280 2560 5120 10240 12800)

for conn in ${conns[@]}
do 
    matrix="perm_128n_${conn}c_s1_2MB.cm"
    echo "Running simulations for connection matrix: ${matrix} with ${conn} connections"
    # Trim
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/def/trim-0.75-nofr/opt-perm${conn}-128n-2MB-trim0.75_nofr-spray-${i}.csv -ratecoef 0.75 -trim -nofr
    done

    # Trim + RTS
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/def/trim-rts-0.75-nofr/opt-perm${conn}-128n-2MB-trimrts0.75_nofr-spray-${i}.csv -ratecoef 0.75 -trim -rts -nofr
    done

    # # Coding
    # for i in {1..10}
    # do
    #     ./htsim_consterase -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/def/coding/opt-perm${conn}-128n-2MB-coding-spray-${i}.csv -ratecoef 0.9 -k 25
    # done

    # # ROCE
    # for i in {1..10}
    # do
    #     ./htsim_roce_new -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/roce0.9/opt-perm${conn}-128n-2MB-roce0.9-spray-${i}.csv -ratecoef 0.9
    # done
done

# TODO: Flowlet and adaptive versions
