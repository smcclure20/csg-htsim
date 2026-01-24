#!/bin/bash

# # ECMP
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-4-2/f9-coding/ecmp/optcoding-ata-128n-2MB-f9-ecmp-${i}.csv -ratecoef 1 -fails 9 -failpct 0.8 -k 0 
# done

# # PLB
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-4-2/f9-coding/plb4-t0.6/optcoding-ata-128n-2MB-f9-plb4_06-${i}.csv -ratecoef 1 -fails 9 -failpct 0.8 -k 0 
# done

# # RR
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-4-2/f9-coding/rr/optcoding-ata-128n-2MB-f9-rr-${i}.csv -ratecoef 1 -fails 9 -failpct 0.8 -k 0 
# done

# # Host spray
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-4-2/f9-coding/spray/optcoding-ata-128n-2MB-f9-spray-${i}.csv -ratecoef 1 -fails 9 -failpct 0.8 -k 0 
# done

# Host spray adaptive
for i in {1..10}
do
    ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb sprayad -of ../../results/opt-4-2/f9-coding/spray-ad/optcoding-ata-128n-2MB-f9-sprayad-${i}.csv -ratecoef 1 -fails 9 -failpct 0.8 -k 0 
done

# # ECMP AR
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp_ar -of ../../results/opt-4-2/f9-coding/ecmp-ar/optcoding-ata-128n-2MB-f9-ecmpar-${i}.csv -ratecoef 1 -fails 9 -failpct 0.8 -k 0 
# done


# # Flowlet AR
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat fl_ar -of ../../results/opt-4-2/f9-coding/flowlet-ar/optcoding-ata-128n-2MB-f9-flowletar-${i}.csv -ratecoef 1 -fails 9 -failpct 0.8 -k 0 
# done

# # Packet AR
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat pkt_ar -of ../../results/opt-4-2/f9-coding/packet-ar/optcoding-ata-128n-2MB-f9-packetar-${i}.csv -ratecoef 1 -fails 9 -failpct 0.8 -k 0 
# done

# # Subflow
# for i in {1..10}
# do
#     ./htsim_consterase -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-4-2/f9-coding/subflow2-ecmp/optcoding-ata-128n-2MB-f9-subflow2-${i}.csv -ratecoef 1 -fails 9 -failpct 0.8 -k 0 -subflows 2
# done
