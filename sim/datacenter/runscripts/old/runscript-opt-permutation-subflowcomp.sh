#!/bin/bash

./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_128c_10s_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-perm128c-128n-2MB-10subflow-spray-flows.csv -ratecoef 0.9 -dup 5
./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_128c_seed1_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-perm128c-128n-2MB-1subflow-spray-flows.csv -ratecoef 0.9