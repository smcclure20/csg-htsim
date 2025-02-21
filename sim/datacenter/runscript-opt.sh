#!/bin/bash

# ./htsim_constcca -end 70000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt/opt-ata-128n-2MB-ecmp-flows-new.csv
# ./htsim_constcca -end 70000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt/opt-ata-128n-2MB-spray-flows.csv
# ./htsim_constcca -end 70000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -of ../../results/opt/opt-xata-128n-2MB-15mrtt-plb-flows.csv
# ./htsim_constcca -end 70000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt/opt-ata-128n-2MB-rr-flows.csv

# ./htsim_constcca -end 70000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt/opt-ata-128n-2MB-f2-ecmp-flows.csv -fails 2
# ./htsim_constcca -end 70000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt/opt-ata-128n-2MB-f2-spray-flows.csv -fails 2
# ./htsim_constcca -end 70000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -of ../../results/opt/opt-ata-128n-2MB-f2-15mrtt-plb-flows.csv -fails 2
# ./htsim_constcca -end 70000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt/opt-ata-128n-2MB-f2-rr-flows.csv -fails 2

./htsim_constcca -end 70000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/opt/opt-ata-128n-2MB-offset-ecmp-flows.csv
./htsim_constcca -end 70000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/opt/opt-ata-128n-2MB-offset-spray-flows.csv
./htsim_constcca -end 70000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -of ../../results/opt/opt-xata-128n-2MB-offset-15mrtt-plb-flows.csv
./htsim_constcca -end 70000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/opt/opt-ata-128n-2MB-offset-rr-flows.csv

# ./htsim_swift -end 150000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/swift2/swift-ata-128n-2MB-ecmp-flows.csv
# ./htsim_swift -end 150000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/swift2/swift-ata-128n-2MB-spray-flows.csv
# ./htsim_swift -end 150000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -of ../../results/swift2/swift-ata-128n-2MB-plb-flows.csv
# ./htsim_swift -end 150000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/swift2/swift-ata-128n-2MB-rr-flows.csv

# ./htsim_swift -end 150000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -of ../../results/swift2/swift-ata-128n-2MB-f2-ecmp-flows.csv -fails 2
# ./htsim_swift -end 150000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb spray -of ../../results/swift2/swift-ata-128n-2MB-f2-spray-flows.csv -fails 2
# ./htsim_swift -end 150000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat ecmp -hostlb plb -of ../../results/swift2/swift-ata-128n-2MB-f2-plb-flows.csv -fails 2
# ./htsim_swift -end 150000 -tm ./connection_matrices/alltoall_128n_2MB.cm -nodes 128 -strat rr -of ../../results/swift2/swift-ata-128n-2MB-f2-rr-flows.csv -fails 2