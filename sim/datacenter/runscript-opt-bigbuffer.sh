#!/bin/bash

./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-bigbuffer/opt-ata-128n-2MB-q1000-f9-ecmp-flows.csv -q 1000 -fails 9 -failpct 0.8
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-bigbuffer/opt-ata-128n-2MB-q1000-f9-spray-flows.csv -q 1000 -fails 9 -failpct 0.8
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -of ../../results/opt-bigbuffer/opt-ata-128n-2MB-q1000-f9-15mrtt-plb-flows.csv -q 1000 -fails 9 -failpct 0.8
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-bigbuffer/opt-ata-128n-2MB-q1000-f9-rr-flows.csv -q 1000 -fails 9 -failpct 0.8

