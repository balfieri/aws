# build.sh - simple build script used by launch that builds main.cpp
#            (this gets executed once on the master_inst)
#
g++ -O3 -o main main.cpp
