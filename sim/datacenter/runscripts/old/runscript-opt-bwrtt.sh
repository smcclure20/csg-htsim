#!/bin/bash

./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt/opt-ata-128n-2MB-1Gb-ecmp-flows.csv -linkspeed 1000
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt/opt-ata-128n-2MB-1Gb-spray-flows.csv -linkspeed 1000
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt/opt-ata-128n-2MB-1Gb-plbecn4-flows.csv -linkspeed 1000
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt/opt-ata-128n-2MB-1Gb-rr-flows.csv -linkspeed 1000

./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt/opt-ata-128n-2MB-1Gb-f2-ecmp-flows.csv -fails 2 -linkspeed 1000
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt/opt-ata-128n-2MB-1Gb-f2-spray-flows.csv -fails 2 -linkspeed 1000
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt/opt-ata-128n-2MB-1Gb-f2-plbecn4-flows.csv -fails 2 -linkspeed 1000
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt/opt-ata-128n-2MB-1Gb-f2-rr-flows.csv -fails 2 -linkspeed 1000

./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt/opt-ata-128n-2MB-1Gb-f9-ecmp-flows.csv -fails 9 -failpct 0.8 -linkspeed 1000
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt/opt-ata-128n-2MB-1Gb-f9-spray-flows.csv -fails 9 -failpct 0.8 -linkspeed 1000
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt/opt-ata-128n-2MB-1Gb-f9-plbecn4-flows.csv -fails 9 -failpct 0.8 -linkspeed 1000
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt/opt-ata-128n-2MB-1Gb-f9-rr-flows.csv -fails 9 -failpct 0.8 -linkspeed 1000