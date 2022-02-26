#!/usr/bin/env python
# coding: utf-8

# In[30]:


import os
import sys
import time
import subprocess

# get # of sockets and cpus
cpu_sockets = int(subprocess.check_output('cat /proc/cpuinfo | grep "physical id" | sort -u | wc -l', shell=True))
cpu_cores = os.cpu_count()

# check if pibench exists, clone and build if necessary
if not os.path.exists("./pibench"):
    os.system("git clone --recursive https://github.com/sfu-dis/pibench.git")
    os.system("cp replace_in_pb.cpp pibench/src/benchmark.cpp")
    os.system("cd pibench && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=0 .. && make")


### Configure parameters

# Modify this according to your own machine configuration
cores = []
for i in range(0, cpu_cores, cpu_sockets):
    cores.append(i)
print(cores)
# cores = [0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,\
# 40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78];

numa_cores = []
for i in range(0, int(cpu_cores/cpu_sockets), cpu_sockets):
    numa_cores.append(i)
for i in range(1, int(cpu_cores/cpu_sockets), cpu_sockets):
    numa_cores.append(i)
print(numa_cores)
# numa_cores = [0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,\
# 1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39];

repeat = 3 # # runs for each point
base_size = 100000000 # each run starts with 100M record base index (load phase)
seconds = 10 # followed by 10 seconds operation (only operation will be measured)


pibench_path = "pibench/build/src/PiBench" # default path to pibench executable
lib_dir = "wrappers/" # default path to binary folder
result_dir = "./results" # default path to folder for which results will be saved into
pool_path = "" # by default PMem pool will be created in current dir

base_command = "numactl --membind=0 sudo LD_PRELOAD=/usr/lib64/libjemalloc.so"


# "HOT","Masstree","ROART_DRAM","ROART_DCMM","ROART_PMDK","FPTree_DRAM","FPTree_PMEM","DPTree","LBTree_DRAM","LBTree_PMEM","PACTree"
Uniform = ["HOT","Masstree","ROART_DRAM","ROART_DCMM","ROART_PMDK","FPTree_DRAM","FPTree_PMEM","DPTree","LBTree_DRAM","LBTree_PMEM","PACTree"] 
uniform_threads = [40,30,20,10,5,1] # [40,30,20,10,5,1]  customize your choice of data points
uniform_ops = ["-r 1","-r 0 -i 1","-r 0 -u 1","-r 0 -s 1"] # "-r 1","-r 0 -i 1","-r 0 -u 1","-r 0 -s 1"

# "HOT","Masstree","ROART_DRAM","ROART_DCMM","ROART_PMDK","FPTree_DRAM","FPTree_PMEM","DPTree","LBTree_DRAM","LBTree_PMEM","PACTree"
Skewed = ["HOT","Masstree","ROART_DRAM","ROART_DCMM","ROART_PMDK","FPTree_DRAM","FPTree_PMEM","DPTree","LBTree_DRAM","LBTree_PMEM","PACTree"] 
skewed_threads = [40,30,20,10,5,1] # [40,30,20,10,5,1]
skewed_ops = ["-r 1","-r 0 -u 1","-r 0 -s 1"] # "-r 1","-r 0 -u 1","-r 0 -s 1"
self_similar = 0.2

# "HOT","Masstree","ROART_DRAM","ROART_DCMM","ROART_PMDK","FPTree_DRAM","FPTree_PMEM","DPTree","LBTree_DRAM","LBTree_PMEM","PACTree"
Mixed = ["HOT","Masstree","ROART_DRAM","ROART_DCMM","ROART_PMDK","FPTree_DRAM","FPTree_PMEM","DPTree","LBTree_DRAM","LBTree_PMEM","PACTree"]
mixed_threads = [40,30,20,10,5,1] # [40,30,20,10,5,1]
mixed_ops = ["-r 0.9 -i 0.1","-r 0.5 -i 0.5","-r 0.1 -i 0.9"] # "-r 0.9 -i 0.1","-r 0.5 -i 0.5","-r 0.1 -i 0.9"

# "HOT","Masstree","ROART_DRAM","ROART_DCMM","ROART_PMDK","FPTree_DRAM","FPTree_PMEM","DPTree","LBTree_DRAM","LBTree_PMEM","PACTree"
Latency = ["HOT","Masstree","ROART_DRAM","ROART_DCMM","ROART_PMDK","FPTree_DRAM","FPTree_PMEM","DPTree","LBTree_DRAM","LBTree_PMEM","PACTree"]
latency_threads = [40,30,20,10,5,1] # [40,30,20,10,5,1]
latency_ops = ["-r 1","-r 0 -i 1","-r 0 -s 1"] # "-r 1","-r 0 -i 1","-r 0 -u 1","-r 0 -s 1"
sampling = 0.1

# "HOT","Masstree","ROART_DRAM","ROART_DCMM","ROART_PMDK","FPTree_DRAM","FPTree_PMEM","DPTree","LBTree_DRAM","LBTree_PMEM","PACTree"
NUMA = ["HOT","Masstree","ROART_DRAM","ROART_DCMM","ROART_PMDK","FPTree_DRAM","FPTree_PMEM","DPTree","LBTree_DRAM","LBTree_PMEM","PACTree"]
numa_threads = [40,30] # [40,30]
numa_ops = ["-r 1","-r 0 -i 1","-r 0 -u 1","-r 0 -s 1"] # "-r 1","-r 0 -i 1","-r 0 -u 1","-r 0 -s 1"

# "FPTree_PMEM","DPTree","LBTree_PMEM"
VarKey = ["FPTree_PMEM","DPTree","LBTree_PMEM"]
varkey_threads = [40,30,20,10,5,1] # [40,30,20,10,5,1]
varkey_ops = ["-r 1","-r 0 -i 1"] # "-r 1","-r 0 -i 1"


# Modify following dictionaries if you changed default pool paths/names or binary names
tree_to_pool = {"HOT":[], "Masstree":[], "ROART_DRAM":[], "ROART_DCMM":["pool"], "ROART_PMDK":["pool"],
"FPTree_DRAM":[], "FPTree_PMEM":["pool"], "DPTree":["pool"], "PACTree":["dl","sl","log"], "LBTree_DRAM":[], "LBTree_PMEM":["pool"]}

tree_to_lib = {"HOT":"libhot_wrapper.so", "Masstree":"libmasstree_wrapper.so", "ROART_DRAM":"libroart_dram.so", 
"ROART_DCMM":"libroart_dcmm.so", "ROART_PMDK":"libroart_pmdk.so", "FPTree_DRAM":"libfptree_dram.so", 
"FPTree_PMEM":"libfptree_pmem.so", "DPTree":"libdptree_pmem.so", "PACTree":"libpactree_pmem.so", 
"LBTree_DRAM":"liblbtree_dram.so", "LBTree_PMEM":"liblbtree_pmem.so"}

tree_to_lib_varkey = {"FPTree_PMEM":"libfptree_pmem_varkey.so", "DPTree":"libdptree_pmem_varkey.so", 
"LBTree_PMEM":"liblbtree_pmem_varkey.so"}




### Benchmark code start

def create_result_folders(tree, exp):
    if not os.path.isdir(result_dir):
        os.mkdir(result_dir);
    tree_dir = result_dir + "/" + tree
    if not os.path.isdir(tree_dir):
        os.mkdir(tree_dir);
    exp_dir = tree_dir + "/" + exp
    if not os.path.isdir(exp_dir):
        os.mkdir(exp_dir);  
    return exp_dir

def create_command(num_thread, cores, tree, op, exp):
    l = []
    for i in range(min(num_thread, len(cores))):
        l.append(str(cores[i]))
    s = "OMP_PLACES=\'{" + ",".join(l) + "}\' OMP_PROC_BIND=TRUE OMP_NESTED=TRUE"
    if exp == "VarKey":
        lib = tree_to_lib_varkey[tree]
    else:
        lib = tree_to_lib[tree]
    command_list = [
        base_command, 
        s, 
        pibench_path, 
        lib_dir + "/" + lib, 
        "-n " + str(base_size), 
        "--mode time --seconds " + str(seconds),
        op,
        "-t " + str(thread)
        ]
    if exp == "Skewed":
        command_list.append("--distribution SELFSIMILAR --skew " + str(self_similar))
    elif exp == "Latency":
        command_list.append("--latency_sampling " + str(sampling))
    if pool_path != "":
        if tree == "PACTree":
            command_list.append("--pool_path=" + pool_path)
        else:
            command_list.append("--pool_path=" + pool_path + "pool")
    return " ".join(command_list)

op_to_filename = {"-r 1":"lookup", "-r 0 -i 1":"insert", "-r 0 -u 1":"update", "-r 0 -s 1":"scan", 
"-r 0.9 -i 0.1":"read_heavy", "-r 0.5 -i 0.5":"balanced", "-r 0.1 -i 0.9":"write_heavy"}

# Uniform
for tree in Uniform:
    exp_dir = create_result_folders(tree, "Uniform") # create result and tree folder if necessary
    for op in uniform_ops:
        file_path = exp_dir + "/" + tree.lower() + "_" + op_to_filename[op] + "_results.txt" # path to result file that will be (re)created
        if os.path.exists(file_path): # remove old result file if exists
            os.remove(file_path)
        for thread in uniform_threads: # for each # thread
            command = create_command(thread, cores, tree, op, "Uniform") + " >> " + file_path
            print(command)
            for i in range(repeat): # repeat runs at each data point
                with open(file_path, 'a') as f:
                    f.write(command + '\n')
                    f.close()
                os.system(command)
                for p in tree_to_pool[tree]:
                    os.system("rm " + pool_path + p)
                time.sleep(2)

# Skewed
for tree in Skewed:
    exp_dir = create_result_folders(tree, "Skewed") # create result and tree folder if necessary
    for op in skewed_ops:
        file_path = exp_dir + "/" + tree.lower() + "_" + op_to_filename[op] + "_results.txt" # path to result file that will be (re)created
        if os.path.exists(file_path): # remove old result file if exists
            os.remove(file_path)
        for thread in skewed_threads: # for each # thread
            command = create_command(thread, cores, tree, op, "Skewed") + " >> " + file_path
            print(command)
            for i in range(repeat): # repeat runs at each data point
                with open(file_path, 'a') as f:
                    f.write(command + '\n')
                    f.close()
                os.system(command)
                for p in tree_to_pool[tree]:
                    os.system("rm " + pool_path + p)
                time.sleep(2)

# Mixed
for tree in Mixed:
    exp_dir = create_result_folders(tree, "Mixed") # create result and tree folder if necessary
    for op in mixed_ops:
        file_path = exp_dir + "/" + tree.lower() + "_" + op_to_filename[op] + "_results.txt" # path to result file that will be (re)created
        if os.path.exists(file_path): # remove old result file if exists
            os.remove(file_path)
        for thread in mixed_threads: # for each # thread
            command = create_command(thread, cores, tree, op, "Mixed") + " >> " + file_path
            print(command)
            for i in range(repeat): # repeat runs at each data point
                with open(file_path, 'a') as f:
                    f.write(command + '\n')
                    f.close()
                os.system(command)
                for p in tree_to_pool[tree]:
                    os.system("rm " + pool_path + p)
                time.sleep(2)

# Latency
for tree in Latency:
    exp_dir = create_result_folders(tree, "Latency") # create result and tree folder if necessary
    for op in latency_ops:
        file_path = exp_dir + "/" + tree.lower() + "_" + op_to_filename[op] + "_results.txt" # path to result file that will be (re)created
        if os.path.exists(file_path): # remove old result file if exists
            os.remove(file_path)
        for thread in latency_threads: # for each # thread
            command = create_command(thread, cores, tree, op, "Latency") + " >> " + file_path
            print(command)
            for i in range(repeat): # repeat runs at each data point
                with open(file_path, 'a') as f:
                    f.write(command + '\n')
                    f.close()
                os.system(command)
                for p in tree_to_pool[tree]:
                    os.system("rm " + pool_path + p)
                time.sleep(2)

# NUMA
for tree in NUMA:
    exp_dir = create_result_folders(tree, "NUMA") # create result and tree folder if necessary
    for op in numa_ops:
        file_path = exp_dir + "/" + tree.lower() + "_" + op_to_filename[op] + "_results.txt" # path to result file that will be (re)created
        if os.path.exists(file_path): # remove old result file if exists
            os.remove(file_path)
        for thread in numa_threads: # for each # thread
            command = create_command(thread, numa_cores, tree, op, "NUMA") + " >> " + file_path
            print(command)
            for i in range(repeat): # repeat runs at each data point
                with open(file_path, 'a') as f:
                    f.write(command + '\n')
                    f.close()
                os.system(command)
                for p in tree_to_pool[tree]:
                    os.system("rm " + pool_path + p)
                time.sleep(2)

# VarKey
for tree in VarKey:
    exp_dir = create_result_folders(tree, "VarKey") # create result and tree folder if necessary
    for op in varkey_ops:
        file_path = exp_dir + "/" + tree.lower() + "_" + op_to_filename[op] + "_results.txt" # path to result file that will be (re)created
        if os.path.exists(file_path): # remove old result file if exists
            os.remove(file_path)
        for thread in varkey_threads: # for each # thread
            command = create_command(thread, cores, tree, op, "VarKey") + " >> " + file_path
            print(command)
            for i in range(repeat): # repeat runs at each data point
                with open(file_path, 'a') as f:
                    f.write(command + '\n')
                    f.close()
                os.system(command)
                for p in tree_to_pool[tree]:
                    os.system("rm " + pool_path + p)
                time.sleep(2)
