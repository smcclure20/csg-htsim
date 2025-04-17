#!/bin/bash
conns=(128) #(128 1280 2560 5120 10240 12800)

for conn in ${conns[@]}
do 
    matrix="perm_128n_${conn}c_s1_2MB.cm"
    echo "Running simulations for connection matrix: ${matrix} with ${conn} connections"
    # ECMP
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -of ../../results/opt-4-2/perm${conn}-f2/ecmp/opt-perm${conn}-128n-2MB-f2-ecmp-${i}.csv -ratecoef 0.9 -fails 2
    done

    # PLB
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-4-2/perm${conn}-f2/plb4-t0.6/opt-perm${conn}-128n-2MB-f2-plb4_06-${i}.csv -ratecoef 0.9 -fails 2
    done

    # RR
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat rr -of ../../results/opt-4-2/perm${conn}-f2/rr/opt-perm${conn}-128n-2MB-f2-rr-${i}.csv -ratecoef 0.9 -fails 2
    done

    # Host spray
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/perm${conn}-f2/spray/opt-perm${conn}-128n-2MB-f2-spray-${i}.csv -ratecoef 0.9 -fails 2
    done

    # ECMP AR
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp_ar -of ../../results/opt-4-2/perm${conn}-f2/ecmp-ar/opt-perm${conn}-128n-2MB-f2-ecmpar-${i}.csv -ratecoef 0.9 -fails 2
    done


    # Flowlet AR
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat fl_ar -of ../../results/opt-4-2/perm${conn}-f2/flowlet-ar/opt-perm${conn}-128n-2MB-f2-flowletar-${i}.csv -ratecoef 0.9 -fails 2
    done

    # Packet AR
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat pkt_ar -of ../../results/opt-4-2/perm${conn}-f2/packet-ar/opt-perm${conn}-128n-2MB-f2-packetar-${i}.csv -ratecoef 0.9 -fails 2
    done

done
