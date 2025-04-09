#!/bin/bash
conns=(10240) #1280 2560 5120 10240 12800

for conn in ${conns[@]}
do 
    matrix="perm_128n_${conn}c_s1_2MB.cm"
    echo "Running simulations for connection matrix: ${matrix} with ${conn} connections"
    # # ECMP
    # for i in {1..10}
    # do
    #     ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -of ../../results/opt-4-2/perm${conn}/ecmp/opt-perm${conn}-128n-2MB-ecmp-${i}.csv -ratecoef 0.9
    # done

    # # PLB
    # for i in {1..10}
    # do
    #     ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-4-2/perm${conn}/plb/opt-perm${conn}-128n-2MB-plb4-${i}.csv -ratecoef 0.9
    # done

    # # RR
    # for i in {1..10}
    # do
    #     ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat rr -of ../../results/opt-4-2/perm${conn}/rr/opt-perm${conn}-128n-2MB-rr-${i}.csv -ratecoef 0.9
    # done

    # # Host spray
    # for i in {1..10}
    # do
    #     ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/perm${conn}/spray/opt-perm${conn}-128n-2MB-spray-${i}.csv -ratecoef 0.9
    # done

    # ECMP AR
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp_ar -of ../../results/opt-4-2/perm${conn}/ecmp-ar/opt-perm${conn}-128n-2MB-ecmpar-${i}.csv -ratecoef 0.9
    done


    # Flowlet AR
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat fl_ar -of ../../results/opt-4-2/perm${conn}/flowlet-ar/opt-perm${conn}-128n-2MB-flowletar-${i}.csv -ratecoef 0.9
    done

    # Packet AR
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat pkt_ar -of ../../results/opt-4-2/perm${conn}/packet-ar/opt-perm${conn}-128n-2MB-packetar-${i}.csv -ratecoef 0.9
    done

done

# TODO: Flowlet and adaptive versions
