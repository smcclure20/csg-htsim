#!/bin/bash

connmatrix="../sim/datacenter/connection_matrices/alltoall_128n_2MB.cm"
# connmatrix="../sim/datacenter/connection_matrices/perm_128n_128c_s1_2MB.cm"
# connmatrix="../sim/datacenter/connection_matrices/perm_128n_128c_2MB_ring.cm"
# connmatrix="../sim/datacenter/connection_matrices/perm_128n_128c_2MB_torring.cm"
# connmatrix="../sim/datacenter/connection_matrices/perm_128n_128c_2MB_podring.cm"
# connmatrix="../sim/datacenter/connection_matrices/perm_128n_128c_2MB_podringol.cm"
# outputdir="../results/opt-10-8/lb-bigbuffer-f1-100-q-ata"
cmtype="ata"

# transtype="constcca"
# reltype="sack"
# rel_details="-sack 32 -nofr -dup 32" 
transtype="consterase"
reltype="erasure"
rel_details="-k 0"

filename="${reltype}-${cmtype}-128n-2MB-buf100"
failpct=-1.0
failcount=1
outputdir="../results/opt-11-6/${cmtype}-${reltype}-bigbuffer-${failcount}f-nq-newjitter/"
# routetimes=(55 200 550 1050 5000 10000 15000)
# routetimes=(55 60 65 70 75 80 95 90)
routetimes=( 55 60 75 100 150 200 )
# routetimes=(5 10 50 100 150 200)
# type="spray"
# strat="ecmp -hostlb spray"
type="pkt-ar"
strat="pkt_ar"

for routetime in "${routetimes[@]}"; do

    failstart=50
    route=$routetime
    weight=$routetime
    sim_dir="${cmtype}-${reltype}-bigbuffer-${failcount}f-$failstart-$route-$weight"
    mkdir $outputdir/$sim_dir
    mkdir $outputdir/$sim_dir/$type

    for i in {1..10}
    do
        ../sim/datacenter/htsim_${transtype} -end 0 -tm ${connmatrix} -nodes 128 -strat ${strat} -of ${outputdir}/${sim_dir}/${type}/${filename}-${type}-${i}.csv -ratecoef 1 -q 100 \
            -failpct ${failpct} -fails ${failcount} -ftype drop -ftime ${failstart} ${route} ${weight} \
            ${rel_details}
    done
done
