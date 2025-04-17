#!/bin/bash
subflows=(2 4 10)

for subflow in "${subflows[@]}"
do

    # # ECMP
    # for i in {1..10}
    # do
    #     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-4-2/subflows/subflows${subflow}/ecmp/opt-ata-128n-2MB-sf${subflow}-ecmp-${i}.csv -ratecoef 0.9 -subflows $subflow
    # done

    # # PLB
    for i in {1..10}
    do
        ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-4-2/subflows/subflows${subflow}/plb/opt-ata-128n-2MB-sf${subflow}-plb4_06-${i}.csv -ratecoef 0.9 -subflows $subflow
    done

    # # RR
    # for i in {1..10}
    # do
    #     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-4-2/subflows/subflows${subflow}/rr/opt-ata-128n-2MB-sf${subflow}-rr-${i}.csv -ratecoef 0.9 -subflows $subflow
    # done

    # # Host spray
    # for i in {1..10}
    # do
    #     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/subflows/subflows${subflow}/spray/opt-ata-128n-2MB-sf${subflow}-spray-${i}.csv -ratecoef 0.9 -subflows $subflow
    # done
done

# TODO: Flowlet and adaptive versions
