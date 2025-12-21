#!/bin/bash

connmatrix="../sim/datacenter/connection_matrices/alltoall_128n_2MB.cm"
# connmatrix="../sim/datacenter/connection_matrices/perm_128n_128c_s1_2MB.cm"
# connmatrix="../sim/datacenter/connection_matrices/perm_128n_128c_2MB_ring.cm"
# connmatrix="../sim/datacenter/connection_matrices/perm_128n_128c_2MB_podring.cm"
# connmatrix="../sim/datacenter/connection_matrices/perm_128n_128c_2MB_podringol.cm"
# connmatrix="../sim/datacenter/connection_matrices/perm_128n_128c_2MB_torring.cm"
# outputdir="../results/opt-10-8/lb-bigbuffer-f1-100-q-ata"
cmtype="ata"

transtype="constcca"
reltype="sack"
rel_details="-sack 32 -nofr -dup 32" 
# rel_details="-k 0"

filename="${reltype}-${cmtype}-128n-2MB-buf100"
failpct=-1.0
failcount=2
failstart=0
route=0
weight=0
# sim_dir="ata-erasure-bigbuffer-4f-$failstart-$route-$weight"
outputdir="../results/opt-11-6/${cmtype}-${reltype}-bigbuffer-${failcount}f-nq/fulltime"
# mkdir $outputdir/$sim_dir
# mkdir $outputdir/$sim_dir/spray
lbtype="sprayad"
strat="ecmp -hostlb sprayad"

# Host spray adaptive
for i in {1..10}
do
    ../sim/datacenter/htsim_${transtype} -end 0 -tm ${connmatrix} -nodes 128 -strat ${strat} -of ${outputdir}/${lbtype}/${filename}-${lbtype}-${i}.csv  -ratecoef 1 -q 100 \
            -failpct ${failpct} -fails ${failcount} -ftype drop -ftime ${failstart} ${route} ${weight} \
            ${rel_details}
done
