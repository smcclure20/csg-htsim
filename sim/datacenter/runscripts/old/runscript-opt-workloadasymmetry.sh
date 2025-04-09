#!/bin/bash

# ./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_128c_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt/opt-perm-128c-128n-2MB-ecmp-flows.csv
# ./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_128c_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt/opt-perm-128c-128n-2MB-plbecn4-flows.csv
# ./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_128c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt/opt-perm-128c-128n-2MB-spray-flows.csv
# ./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_128c_2MB.cm -nodes 128 -strat rr -of ../../results/opt/opt-perm-128c-128n-2MB-rr-flows.csv

# ./htsim_constcca -end 0 -tm ./connection_matrices/rand_128n_1280c_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt/opt-rand-1280c-128n-2MB-ecmp-flows.csv
# ./htsim_constcca -end 0 -tm ./connection_matrices/rand_128n_1280c_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt/opt-rand-1280c-128n-2MB-plbecn4-flows.csv
# ./htsim_constcca -end 0 -tm ./connection_matrices/rand_128n_1280c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt/opt-rand-1280c-128n-2MB-spray-flows.csv
# ./htsim_constcca -end 0 -tm ./connection_matrices/rand_128n_1280c_2MB.cm -nodes 128 -strat rr -of ../../results/opt/opt-rand-1280c-128n-2MB-rr-flows.csv

./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_1280c_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt/opt-perm-1280c-128n-2MB-ecmp-flows.csv
./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_1280c_2MB.cm -nodes 128 -strat ecmp -hostlb plb -plbecn 4 -of ../../results/opt/opt-perm-1280c-128n-2MB-plbecn4-flows.csv
./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_1280c_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt/opt-perm-1280c-128n-2MB-spray-flows.csv
./htsim_constcca -end 0 -tm ./connection_matrices/perm_128n_1280c_2MB.cm -nodes 128 -strat rr -of ../../results/opt/opt-perm-1280c-128n-2MB-rr-flows.csv


