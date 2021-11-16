#!/bin/bash
# A bash script to run the benchmark and save the log files

# shellcheck disable=SC2164
cd build
make -j4
cd ../visualize

# ../build/t_manip
../build/t_manip "PathOptimizerKOMO"
# # .build/t_manip "PathSimplifier"
# ../build/t_manip "RRTstar"

