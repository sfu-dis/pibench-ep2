#!/bin/bash

# Script for running pmem/dram index bechmark on Duration-based pibench

# Steps for using this script:
# Copy this script to folder that contains Pibench executable
# 1. Set the index operation type (op)
# 2. Set tree
# 3. Set pool_name, if testing dram index, set it to ""
# 4. Set binary_path to be the relative/absolute path to .so file
# 5. Save, exit and run sudo ./tb_bench.sh (It is important that you run with sudo)
# 6. Output will be appended to a txt file of format tree_op_result.txt


# Arguments

op="-r 1"
#op="-r 0 -i 1"
#op="-r 0 -u 1"
#op="-r 0 -s 1"

tree="hot" # name of index (masstree/hot/fptree/lbtree/dptree/roart/utree etc)

pool_name="" # name of pool file to delete (assume on pmem0), set to "" if running dram index

binary_path="../../../../index-wrappers/Hot/hot/build/libhot_wrapper.so"

# Script

echo "Benchmark type: ${op}"

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

  # "-r 0 -d 1")
  #   type="_delete_results.txt"
  #   ;;

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


for t in 1 5 15 20 30 40
do
	n=100000000

	command="hog-machine.sh numactl --cpunodebind=0 --membind=0 sudo LD_PRELOAD=/usr/lib64/libjemalloc.so \
./PiBench ${binary_path} -n ${n} ${op} --mode time --time 10 -t ${t} >> ${file}"

	# for i in 1 2 # 2 trials
	# do
		echo "${command}"
		echo -e "\n${command}\n" >> "${file}"
		eval "${command}"
		sleep 1
		if [ "${pool_name}" != "" ]; then
			eval "sudo rm ${pool_name}"
		fi
		sleep 3
	# done
done