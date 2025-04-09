#!/bin/bash
conns=(128 1280 2560 5120 10240 12800)

for conn in ${conns[@]}
do 
    matrix="perm_128n_${conn}c_s1_2MB.cm"
    echo "Running simulations for connection matrix: ${matrix} with ${conn} connections"
    # Trim
    # for i in {1..10}
    # do
    #     ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/trim/opt-perm${conn}-128n-2MB-trim-spray-${i}.csv -ratecoef 0.9
    # done

    # # Trim + RTS
    # for i in {1..10}
    # do
    #     ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/trim-rts/opt-perm${conn}-128n-2MB-trimrts-spray-${i}.csv -ratecoef 0.9
    # done

    # # Coding
    # for i in {1..10}
    # do
    #     ./htsim_consterase -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/coding/opt-perm${conn}-128n-2MB-coding-spray-${i}.csv -ratecoef 0.9
    # done

    # ROCE
    for i in {1..10}
    do
        ./htsim_roce_new -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/roce/opt-perm${conn}-128n-2MB-roce-spray-${i}.csv -ratecoef 1
    done
done

# TODO: Flowlet and adaptive versions
