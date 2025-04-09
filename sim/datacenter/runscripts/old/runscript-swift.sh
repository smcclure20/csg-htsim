#!/bin/bash

./htsim_swift -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/swift2/swift2-ata-128n-2MB-ecmp-flows.csv
./htsim_swift -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/swift2/swift2-ata-128n-2MB-spray-flows.csv
./htsim_swift -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -of ../../results/swift2/swift2-ata-128n-2MB-plb-flows.csv
./htsim_swift -end 0 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/swift2/swift2-ata-128n-2MB-rr-flows.csv
