#!/bin/bash
conns=(128 1280 2560 5120 10240 12800)

for conn in ${conns[@]}
do 
    matrix="perm_128n_${conn}c_s1_2MB.cm"
    echo "Running simulations for connection matrix: ${matrix} with ${conn} connections"
    # # ECMP
    # for i in {1..10}
    # do
    #     ./htsim_consterase -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -of ../../results/opt-4-2/perm${conn}-coding/ecmp/optcoding-perm${conn}-128n-2MB-ecmp-${i}.csv -ratecoef 1 -k 0 
    # done

    # # PLB
    # for i in {1..10}
    # do
    #     ./htsim_consterase -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-4-2/perm${conn}-coding/plb4-t0.6/optcoding-perm${conn}-128n-2MB-plb4_06-${i}.csv -ratecoef 1 -k 0 
    # done

    # # RR
    # for i in {1..10}
    # do
    #     ./htsim_consterase -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat rr -of ../../results/opt-4-2/perm${conn}-coding/rr/optcoding-perm${conn}-128n-2MB-rr-${i}.csv -ratecoef 1 -k 0 
    # done

    # # Host spray
    # for i in {1..10}
    # do
    #     ./htsim_consterase -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/perm${conn}-coding/spray/optcoding-perm${conn}-128n-2MB-spray-${i}.csv -ratecoef 1 -k 0 
    # done

        # Host spray adaptive
    for i in {1..10}
    do
        ./htsim_consterase -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -hostlb sprayad -of ../../results/opt-4-2/perm${conn}-coding/spray-ad/optcoding-perm${conn}-128n-2MB-sprayad-${i}.csv -ratecoef 1 -k 0 
    done

    # # ECMP AR
    # for i in {1..10}
    # do
    #     ./htsim_consterase -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp_ar -of ../../results/opt-4-2/perm${conn}-coding/ecmp-ar/optcoding-perm${conn}-128n-2MB-ecmpar-${i}.csv -ratecoef 1 -k 0 
    # done


    # # Flowlet AR
    # for i in {1..10}
    # do
    #     ./htsim_consterase -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat fl_ar -of ../../results/opt-4-2/perm${conn}-coding/flowlet-ar/optcoding-perm${conn}-128n-2MB-flowletar-${i}.csv -ratecoef 1 -k 0 
    # done

    # # Packet AR
    # for i in {1..10}
    # do
    #     ./htsim_consterase -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat pkt_ar -of ../../results/opt-4-2/perm${conn}-coding/packet-ar/optcoding-perm${conn}-128n-2MB-packetar-${i}.csv -ratecoef 1 -k 0 
    # done

    # # Subflow 
    # for i in {1..10}
    # do
    #     ./htsim_consterase -end 0 -tm ./connection_matrices/${matrix} -nodes 128 -strat ecmp -of ../../results/opt-4-2/perm${conn}-coding/subflow2-ecmp/optcoding-ata-128n-2MB-sf2-ecmp-${i}.csv -ratecoef 1 -subflows 2 -k 0 
    # done

done
