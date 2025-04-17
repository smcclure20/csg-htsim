#!/bin/bash
conns=(128 1280 2560 5120 10240 12800)
fails=4
failpct=0.6

for conn in ${conns[@]}
do 
    matrix="perm_128n_${conn}c_s1_2MB.cm"
    echo "Running simulations for connection matrix: ${matrix} with ${conn} connections"
    #     # TCP
    # for i in {1..10}
    # do
    #     ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/f${fails}/tcp/opt-perm${conn}-128n-2MB-f${fails}-tcp-spray-${i}.csv -ratecoef 0.9 -fails ${fails} -failpct ${failpct}
    # done

    # Trim
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/f${fails}/trim-0.75-nofr/opt-perm${conn}-128n-2MB-f${fails}-trim0.75_nofr-spray-${i}.csv -ratecoef 0.75 -trim -nofr -fails ${fails} -failpct ${failpct}
    done

    # Trim + RTS
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/f${fails}/trim-rts-0.75-nofr/opt-perm${conn}-128n-2MB-f${fails}-trimrts0.75_nofr-spray-${i}.csv -ratecoef 0.75 -trim -rts -nofr -fails ${fails} -failpct ${failpct}
    done

    # # Coding
    # for i in {1..10}
    # do
    #     ./htsim_consterase -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/f${fails}/coding/opt-perm${conn}-128n-2MB-f${fails}-coding-spray-${i}.csv -ratecoef 0.9 -fails ${fails} -failpct ${failpct} -k 25
    # done

    # ROCE
    for i in {1..10}
    do
        ./htsim_roce_new -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/f${fails}/roce0.9/opt-perm${conn}-128n-2MB-f${fails}-roce0.9-spray-${i}.csv -ratecoef 0.9 -fails ${fails} -failpct ${failpct}
    done

    # # Subflow setup
    # for i in {1..10}
    # do
    #     ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/f${fails}/sfd3/opt-perm${conn}-128n-2MB-f${fails}-sfd3-spray-${i}.csv -ratecoef 0.9 -fails ${fails} -failpct ${failpct} -subflows 0 -dup 3
    # done

    # for i in {1..10}
    # do
    #     ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/reliability/perm${conn}/f${fails}/sfd6/opt-perm${conn}-128n-2MB-f${fails}-sfd6-spray-${i}.csv -ratecoef 0.9 -fails ${fails} -failpct ${failpct} -subflows 0 -dup 6
    # done
done
