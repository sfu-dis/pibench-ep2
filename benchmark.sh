#!/bin/bash

# Script for running pmem index bechmark on pibench

# Arguments
op="-r 1"
#op="-r 0 -i 1"
#op="-r 0 -u 1"
#op="-r 0 -d 1"
#op="-r 0 -s 1"

base_tp=1200000 # expected throughput for single thread

output="result.txt" # output file name

pool_name="pool" # pool to delete, set to "" if running dram index

binary_path="../../../index-wrappers/DPTree/build/libdptree_pibench_wrapper.so"

# Script

echo "Benchmark type: ${op}"

echo "Expected single thred throughput: ${base_tp}"

echo "Output file name: ${output}"

echo "Index binary path: ${binary_path}"

command="hog-machine.sh numactl --cpunodebind=0 --membind=0 sudo ./PiBench ${binary_path} "

command2="hog-machine.sh sudo ./PiBench ${binary_path} "

for t in 80 60 40 30 20 15 5 1
do
	for i in 1 2 3 # 3 trials
	do
		if [ $t -gt 40 ]; then
			p=$((base_tp*3*t))	# Remote-access tends to be slow
			n=100000000
			if [[ $p -gt n && "${op}" == "-r 0 -d 1" ]]; then
				n=$p
			fi
			cmd="${command2} -n ${n} -p ${p} ${op} -t ${t} >> ${output}"
			eval "${cmd}"
		else
			p=$((base_tp*9*t))	# Remote-access tends to be slow
			n=100000000
			if [[ $p -gt n && "${op}" == "-r 0 -d 1" ]]; then
				n=$p
			fi
			cmd="${command} -n ${n} -p ${p} ${op} -t ${t} >> ${output}"
			eval "${cmd}"
		fi
		sleep 1
		if [ "${pool_name}" != "" ]; then
			eval "sudo rm ${pool_name}"
		fi
		sleep 3
	done
done