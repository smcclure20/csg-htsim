#!/bin/bash

./htsim_swift  -end 100000 -tm ./connection_matrices/rand_1024_10xconn_2MB_offset.cm  -nodes 1024 -strat ecmp -of ../../results/swift/1024n-rand10x-2mb-offset-ecmp-plb-flows.csv -hostlb plb
./htsim_swift  -end 100000 -tm ./connection_matrices/rand_1024_10xconn_2MB_offset.cm  -nodes 1024 -strat ecmp -of ../../results/swift/1024n-rand10x-2mb-offset-ecmp-hostspray-flows.csv -hostlb spray
./htsim_swift  -end 100000 -tm ./connection_matrices/rand_1024_10xconn_2MB_offset.cm  -nodes 1024 -strat ecmp -of ../../results/swift/1024n-rand10x-2mb-offset-ecmp-flows.csv
./htsim_swift  -end 100000 -tm ./connection_matrices/rand_1024_10xconn_2MB_offset.cm  -nodes 1024 -strat rr -of ../../results/swift/1024n-rand10x-2mb-offset-rr-flows.csv

./htsim_swift  -end 100000 -tm ./connection_matrices/rand_1024_10xconn_2MB_sync.cm  -nodes 1024 -strat ecmp -of ../../results/swift/1024n-rand10x-2mb-ecmp-plb-flows.csv -hostlb plb
./htsim_swift  -end 100000 -tm ./connection_matrices/rand_1024_10xconn_2MB_sync.cm  -nodes 1024 -strat ecmp -of ../../results/swift/1024n-rand10x-2mb-ecmp-hostspray-flows.csv -hostlb spray
./htsim_swift  -end 100000 -tm ./connection_matrices/rand_1024_10xconn_2MB_sync.cm  -nodes 1024 -strat ecmp -of ../../results/swift/1024n-rand10x-2mb-ecmp-flows.csv
./htsim_swift  -end 100000 -tm ./connection_matrices/rand_1024_10xconn_2MB_sync.cm  -nodes 1024 -strat rr -of ../../results/swift/1024n-rand10x-2mb-rr-flows.csv

