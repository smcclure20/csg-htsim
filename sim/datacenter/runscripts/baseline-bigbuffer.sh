#!/bin/bash

# # ECMP
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-4-2/bigbuffer/ecmp/opt-ata-128n-2MB-buf100-ecmp-${i}.csv -ratecoef 0.9 -q 100
# done

# PLB
for i in {1..10}
do
    ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-4-2/bigbuffer/plb4-t0.6/opt-ata-128n-2MB-buf100-plb4_06-${i}.csv -ratecoef 0.9 -q 100
done

# # RR
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-4-2/bigbuffer/rr/opt-ata-128n-2MB-buf100-rr-${i}.csv -ratecoef 0.9 -q 100
# done

# # Host spray
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/bigbuffer/spray/opt-ata-128n-2MB-buf100-spray-${i}.csv -ratecoef 0.9 -q 100
# done

# # ECMP AR
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp_ar -of ../../results/opt-4-2/bigbuffer/ecmp-ar/opt-ata-128n-2MB-buf100-buf100-ecmpar-${i}.csv -ratecoef 0.9 -q 100
# done


# # Flowlet AR
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat fl_ar -of ../../results/opt-4-2/bigbuffer/flowlet-ar/opt-ata-128n-2MB-buf100-flowletar-${i}.csv -ratecoef 0.9 -q 100
# done

# # Packet AR
# for i in {1..10}
# do
#     ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat pkt_ar -of ../../results/opt-4-2/bigbuffer/packet-ar/opt-ata-128n-2MB-buf100-packetar-${i}.csv -ratecoef 0.9 -q 100
# done
