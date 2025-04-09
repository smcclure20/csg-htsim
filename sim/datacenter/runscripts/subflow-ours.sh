#!/bin/bash

conns=(128 1280 2560 5120 10240 12800)

for conn in ${conns[@]}
do 
    matrix="perm_128n_${conn}c_s1_2MB.cm"

    # Host spray
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/tcp-sf/perm${conn}/spray/d3/opt-ata-128n-2MB-sf0d3-spray-${i}.csv -ratecoef 0.9 -subflows 0 -dup 3
    done

        for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/tcp-sf/perm${conn}/spray/d6/opt-ata-128n-2MB-sf0d6-spray-${i}.csv -ratecoef 0.9 -subflows 0 -dup 6
    done

done

# TODO: Flowlet and adaptive versions
