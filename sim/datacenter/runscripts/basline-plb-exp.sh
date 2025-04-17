#!/bin/bash

# # ECMP
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-4-2/baseline/ecmp/opt-ata-128n-2MB-ecmp-${i}.csv -ratecoef 0.9
# done

# PLB
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-4-2/baseline/plb4-t0.6/opt-ata-128n-2MB-plb4_06-${i}.csv -ratecoef 0.9
done

# # RR
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-4-2/baseline/rr/opt-ata-128n-2MB-rr-${i}.csv -ratecoef 0.9
# done

# # Host spray
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/baseline/spray/opt-ata-128n-2MB-spray-${i}.csv -ratecoef 0.9
# done

# # ECMP AR
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp_ar -of ../../results/opt-4-2/baseline/ecmp-ar/opt-ata-128n-2MB-ecmpar-${i}.csv -ratecoef 0.9
# done


# # Flowlet AR
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat fl_ar -of ../../results/opt-4-2/baseline/flowlet-ar/opt-ata-128n-2MB-flowletar-${i}.csv -ratecoef 0.9
# done

# # Packet AR
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat pkt_ar -of ../../results/opt-4-2/baseline/packet-ar/opt-ata-128n-2MB-packetar-${i}.csv -ratecoef 0.9
# done
