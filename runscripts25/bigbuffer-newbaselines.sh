#!/bin/bash

connmatrix="../sim/datacenter/connection_matrices/alltoall_128n_2MB.cm"
# connmatrix="../sim/datacenter/connection_matrices/perm_128n_128c_s1_2MB.cm"
# connmatrix="../sim/datacenter/connection_matrices/perm_128n_128c_2MB_ring.cm"
# connmatrix="../sim/datacenter/connection_matrices/perm_128n_128c_2MB_podring.cm"
# connmatrix="../sim/datacenter/connection_matrices/perm_128n_128c_2MB_torring.cm"
# connmatrix="../sim/datacenter/connection_matrices/perm_128n_128c_2MB_podringol.cm"
# outputdir="../results/opt-10-8/lb-bigbuffer-f1-100-q-ata"
outputdir="../results/opt-11-6/newbaselines/ata-newjitter"
filename="optcoding-ata-128n-2MB-buf100"

# ECMP will never complete if failpct is 0.0 (since routes are not updated)
# # ECMP
# for i in {1..10}
# do
#     ../sim/datacenter/htsim_consterase -end 0 -tm ${connmatrix} -nodes 128 -strat ecmp -of ${outputdir}/ecmp/${filename}-ecmp-${i}.csv -ratecoef 1 -q 100 -k 0
# done

#PLB # Need to add response to timers for this to work in a scenario without queueing
# for i in {1..10}
# do
#     ../sim/datacenter/htsim_consterase -end 0 -tm ${connmatrix} -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ${outputdir}/plb4-t0.5-to/${filename}-plb4_05-${i}.csv -ratecoef 1 -q 100 -k 0 #
# done

# # RR
# for i in {1..10}
# do
#     ../sim/datacenter/htsim_consterase -end 0 -tm ${connmatrix} -nodes 128 -strat rr -of ${outputdir}/rr/${filename}-rr-${i}.csv -ratecoef 1 -q 100 -k 0
# done

# Host spray
for i in {1..10}
do
    ../sim/datacenter/htsim_consterase -end 0 -tm ${connmatrix} -nodes 128 -strat ecmp -hostlb spray -of ${outputdir}/spray/${filename}-spray-${i}.csv -ratecoef 1 -q 100 -k 0
done

# Host spray adaptive
for i in {1..10}
do
    ../sim/datacenter/htsim_consterase -end 0 -tm ${connmatrix} -nodes 128 -strat ecmp -hostlb sprayad -of ${outputdir}/sprayad/${filename}-sprayad-${i}.csv -ratecoef 1 -q 100 -k 0 #
done

# # ECMP AR
# for i in {1..10}
# do
#     ../sim/datacenter/htsim_consterase -end 0 -tm ${connmatrix} -nodes 128 -strat ecmp_ar -of ${outputdir}/ecmp-ar/${filename}-ecmpar-${i}.csv -ratecoef 1 -q 100 -k 0
# done


# # Flowlet AR # Only reroutes with queueing
# for i in {1..10}
# do
#     ../sim/datacenter/htsim_consterase -end 0 -tm ${connmatrix} -nodes 128 -strat fl_ar -of ${outputdir}/flowlet-ar/${filename}-flowletar-${i}.csv -ratecoef 1 -q 100 -k 0
# done

# Packet AR
for i in {1..10}
do
    ../sim/datacenter/htsim_consterase -end 0 -tm ${connmatrix} -nodes 128 -strat pkt_ar -of ${outputdir}/pkt-ar/${filename}-packetar-${i}.csv -ratecoef 1 -q 100 -k 0 #-failpct ${failpct} -fails ${failcount}
done
