#!/bin/bash

# # ECMP
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-4-2/bigbuffer-coding/ecmp/optcoding-ata-128n-2MB-buf100-ecmp-${i}.csv -ratecoef 1 -q 100 -k 0
# done

# # PLB
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-4-2/bigbuffer-coding/plb4-t0.6/optcoding-ata-128n-2MB-buf100-plb4_06-${i}.csv -ratecoef 1 -q 100 -k 0
# done

# # RR
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-4-2/bigbuffer-coding/rr/optcoding-ata-128n-2MB-buf100-rr-${i}.csv -ratecoef 1 -q 100 -k 0
# done

# # Host spray
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/bigbuffer-coding/spray/optcoding-ata-128n-2MB-buf100-spray-${i}.csv -ratecoef 1 -q 100 -k 0
# done

# Host spray adaptive
for i in {1..10}
do
    ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb sprayad -of ../../results/opt-4-2/bigbuffer-coding/spray-ad/optcoding-ata-128n-2MB-buf100-sprayad-${i}.csv -ratecoef 1 -q 100 -k 0
done

# # ECMP AR
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp_ar -of ../../results/opt-4-2/bigbuffer-coding/ecmp-ar/optcoding-ata-128n-2MB-buf100-buf100-ecmpar-${i}.csv -ratecoef 1 -q 100 -k 0
# done


# # Flowlet AR
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat fl_ar -of ../../results/opt-4-2/bigbuffer-coding/flowlet-ar/optcoding-ata-128n-2MB-buf100-flowletar-${i}.csv -ratecoef 1 -q 100 -k 0
# done

# # Packet AR
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat pkt_ar -of ../../results/opt-4-2/bigbuffer-coding/packet-ar/optcoding-ata-128n-2MB-buf100-packetar-${i}.csv -ratecoef 1 -q 100 -k 0
# done
