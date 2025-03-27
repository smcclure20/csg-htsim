#!/bin/bash

# Base case
# ./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_1280c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-19/opt-perm1280c-128n-2MB-trimrts-spray-flows.csv -ratecoef 0.9 -trim -rts
# ./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_1280c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-19/opt-perm1280c-128n-2MB-trim-spray-flows.csv -ratecoef 0.9 -trim
# ./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_1280c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-19/opt-perm1280c-128n-2MB-spray-flows.csv -ratecoef 0.9 
# ./htsim_consterase -end 0 -tm ./connection_matrices/perm_128n_1280c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-19/opt-perm1280c-128n-2MB-erase-spray-flows.csv -ratecoef 0.9

./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_128c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-perm128c-128n-2MB-trimrts-spray-flows.csv -ratecoef 0.9 -trim -rts
./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_128c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-perm128c-128n-2MB-trim-spray-flows.csv -ratecoef 0.9 -trim
./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_128c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-perm128c-128n-2MB-spray-flows.csv -ratecoef 0.9 
./htsim_consterase -end 0 -tm ./connection_matrices/perm_128n_128c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-perm128c-128n-2MB-erase-spray-flows.csv -ratecoef 0.9

./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_2560c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-perm2560c-128n-2MB-trimrts-spray-flows.csv -ratecoef 0.9 -trim -rts
./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_2560c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-perm2560c-128n-2MB-trim-spray-flows.csv -ratecoef 0.9 -trim
./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_2560c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-perm2560c-128n-2MB-spray-flows.csv -ratecoef 0.9 
./htsim_consterase -end 0 -tm ./connection_matrices/perm_128n_2560c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-perm2560c-128n-2MB-erase-spray-flows.csv -ratecoef 0.9

./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_5120c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-perm5120c-128n-2MB-trimrts-spray-flows.csv -ratecoef 0.9 -trim -rts
./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_5120c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-perm5120c-128n-2MB-trim-spray-flows.csv -ratecoef 0.9 -trim
./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_5120c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-perm5120c-128n-2MB-spray-flows.csv -ratecoef 0.9 
./htsim_consterase -end 0 -tm ./connection_matrices/perm_128n_5120c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-perm5120c-128n-2MB-erase-spray-flows.csv -ratecoef 0.9

./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_10240c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-perm10240c-128n-2MB-trimrts-spray-flows.csv -ratecoef 0.9 -trim -rts
./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_10240c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-perm10240c-128n-2MB-trim-spray-flows.csv -ratecoef 0.9 -trim
./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_10240c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-perm10240c-128n-2MB-spray-flows.csv -ratecoef 0.9 
./htsim_consterase -end 0 -tm ./connection_matrices/perm_128n_10240c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt-3-25/opt-perm10240c-128n-2MB-erase-spray-flows.csv -ratecoef 0.9
