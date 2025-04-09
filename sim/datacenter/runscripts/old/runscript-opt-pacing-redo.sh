#!/bin/bash

# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-pacing90/optp90-ata-128n-2MB-ecmp-flows.csv -ratecoef 0.9 
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-pacing90/optp90-ata-128n-2MB-spray-flows.csv -ratecoef 0.9
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-pacing90/optp90-ata-128n-2MB-plbecn4-flows.csv -ratecoef 0.9
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-pacing90/optp90-ata-128n-2MB-rr-flows.csv -ratecoef 0.9

# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-pacing90/optp90-ata-128n-2MB-f2-ecmp-flows.csv -ratecoef 0.9 -fails 2
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-pacing90/optp90-ata-128n-2MB-f2-spray-flows.csv -ratecoef 0.9 -fails 2
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-pacing90/optp90-ata-128n-2MB-f2-plbecn4-flows.csv -ratecoef 0.9 -fails 2
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-pacing90/optp90-ata-128n-2MB-f2-rr-flows.csv -ratecoef 0.9 -fails 2

# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt-pacing90/optp90-ata-128n-2MB-f9-ecmp-flows.csv -ratecoef 0.9 -fails 9 -failpct 0.8
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-pacing90/optp90-ata-128n-2MB-f9-spray-flows.csv -ratecoef 0.9 -fails 9 -failpct 0.8
./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt-pacing90/optp90-ata-128n-2MB-f9-plbecn4-flows.csv -ratecoef 0.9 -fails 9 -failpct 0.8
# ./htsim_constcca -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt-pacing90/optp90-ata-128n-2MB-f9-rr-flows.csv -ratecoef 0.9 -fails 9 -failpct 0.8