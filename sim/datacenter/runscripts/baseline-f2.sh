#!/bin/bash

# # ECMP
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-4-2/f2/ecmp/opt-ata-128n-2MB-f2-ecmp-${i}.csv -ratecoef 0.9 -fails 2
# done

# # PLB
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-4-2/f2/plb/opt-ata-128n-2MB-f2-plb4_06-${i}.csv -ratecoef 0.9 -fails 2
# done

# # RR
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-4-2/f2/rr/opt-ata-128n-2MB-f2-rr-${i}.csv -ratecoef 0.9 -fails 2
# done

# # Host spray
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/f2/spray/opt-ata-128n-2MB-f2-spray-${i}.csv -ratecoef 0.9 -fails 2
# done

# ECMP AR
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp_ar -of ../../results/opt-4-2/f2/ecmp-ar/opt-ata-128n-2MB-f2-ecmpar-${i}.csv -ratecoef 0.9 -fails 2
done


# Flowlet AR
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat fl_ar -of ../../results/opt-4-2/f2/flowlet-ar/opt-ata-128n-2MB-f2-flowletar-${i}.csv -ratecoef 0.9 -fails 2
done

# Packet AR
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat pkt_ar -of ../../results/opt-4-2/f2/packet-ar/opt-ata-128n-2MB-f2-packetar-${i}.csv -ratecoef 0.9 -fails 2
done
