#!/bin/bash

# Script for running pmem/dram index bechmark on Duration-based pibench

# Steps for using this script:
# 1. Set the index operation type (op)
# 2. Set base_tp only if op="-r 0 -d 1"
# 3. Set tree
# 4. Set pool_name, if testing dram index, set it to ""
# 5. Set binary_path to be the relative path to .so file
# 6. Save, exit and run sudo ./benchmark.sh (It is important that you run with sudo)
# 7. Output will be appended to a txt file of format tree_op_result.txt


# Arguments

op="-r 1"
#op="-r 0 -i 1"
#op="-r 0 -u 1"
#op="-r 0 -d 1"
#op="-r 0 -s 1"

base_tp=1700000 # expected throughput/s for single thread delete operation, only used for delete

tree="hot" # name of index (masstree/hot/fptree/lbtree/dptree/roart/utree etc)

pool_name="" # name of pool file to delete (assume on pmem0), set to "" if running dram index

binary_path="../../../../index-wrappers/Hot/hot/build/libhot_wrapper.so"


# Script

echo "Benchmark type: ${op}"

if [ "${op}" == "-r 0 -d 1" ]; then
	echo "Expected single thread delete throughput: ${base_tp}"
fi

type=""

case $op in

  "-r 1")
    type="_read_results.txt"
    ;;

  "-r 0 -i 1")
    type="_insert_results.txt"
    ;;

  "-r 0 -u 1")
    type="_update_results.txt"
    ;;

  "-r 0 -d 1")
    type="_delete_results.txt"
    ;;

  "-r 0 -s 1")
    type="_scan_results.txt"
    ;;

  *)
    echo -n "Unknown benchmark type!"
    ;;
esac

file="${tree}${type}"

echo "Output file name: ${file}"

if [ "${pool_name}" != "" ]; then
	echo "Pool name: ${pool_name}"
fi

echo "Index binary path: ${binary_path}"

omp_places="0 ,2 ,4 ,6 ,8 ,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,\
1 ,3 ,5 ,7 ,9 ,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,\
40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,\
41,43,45,47,49,51,53,55,57,59,61,63,65,67,69,71,73,75,77,79"


for t in 80 60 40 30 20 15 5 1
do
	length=$((t*3-1));
	omp_command="OMP_PLACES=\"{${omp_places:0:length}}\" OMP_PROC_BIND=TRUE"
	
	if [ $t -gt 20 ]; then
		numa_command="numactl --interleave=0,1"
	else
		numa_command="numactl --membind=0"
	fi

	n=100000000

	if [[ "${op}" == "-r 0 -d 1" && $((base_tp*t*10)) -gt $n ]]; then
		n=$((base_tp*t*10))
	fi
	p=$n

	command="hog-machine.sh ${numa_command} sudo ${omp_command} ./PiBench ${binary_path} \
-n ${n} -p ${p} ${op} --mode time --time 10 -t ${t} >> ${file}"

	for i in 1 2 # 2 trials
	do
		echo "${command}"
		echo -e "\n${command}\n" >> file
		eval "${command}"
		sleep 1
		if [ "${pool_name}" != "" ]; then
			eval "sudo rm ${pool_name}"
		fi
		sleep 3
	done
done