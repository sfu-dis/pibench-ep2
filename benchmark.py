#!/usr/bin/env python
# coding: utf-8

# In[30]:


import os

def define_output_name(op, tree):
    output_file = tree
    if op == "-r 1":
        output_file += "_read_results.txt"
    elif op == "-r 0 -i 1":
        output_file += "_insert_results.txt"
    elif op == "-r 0 -u 1":
        output_file += "_update_results.txt"
    elif op == "-r 0 -d 1":
        output_file += "_delete_results.txt"
    elif op == "-r 0 -s 1":
        output_file += "_scan_results.txt"
    elif op == "-r 0.1 -i 0.9":
        output_file += "_read0.1_insert0.9_results.txt"
    elif op == "-r 0.5 -i 0.5":
        output_file += "_read0.5_insert0.5_results.txt"
    elif op == "-r 0.9 -i 0.1":
        output_file += "_read0.9_insert0.1_results.txt"
    else: 
        os.system("echo Unknown benchmark type!") 
    return output_file

def default_pool_name(mode, tree):
    if mode == "dram":
        return ""
    if tree == "fptree":
        return "test_pool"
    elif tree == "roart" or tree == "roart_dcmm":
        return "pool.data"
    else:
        return "pool"
    
def default_binary_path(mode, tree):
    parent_path = "./wrappers/" + mode + "_wrappers/"
    if tree == "fptree":
        return parent_path + "libfptree_pibench_wrapper.so"
    elif tree == "lbtree":
        return parent_path + "liblbtree_wrapper.so"
    elif tree == "roart":
        return parent_path + "libroart_wrapper.so"
    elif tree == "roart_dcmm":
        return parent_path + "libroart_dcmm_wrapper.so"
    elif tree == "dptree":
        return parent_path + "libdptree_pibench_wrapper.so"
    elif tree == "utree":
        return parent_path + "libutree_pibench_wrapper.so"
    elif tree == "hot":
        return parent_path + "libhot_wrapper.so"
    elif tree == "masstree":
        return parent_path + "libmasstree_wrapper.so"
    else:
        return "unknow tree"
    
def define_dptree_cpubind(threads):
    cpubind = list(range(0, threads * 2, 2))
    cpubind = list(map(str, cpubind))
    str_cpubind = ','.join(cpubind)
    return str_cpubind
    
OPs = ["-r 1", "-r 0 -i 1", "-r 0 -u 1", "-r 0 -s 1"]
TREE = "dptree" # name of index choose from [masstree, hot, fptree, lbtree, dptree, roart, roart_dcmm, utree]
MODE = "pmem" # dram or pmem tree DRAM: [fptree, lbtree, roart, masstree, hot] PMEM: [fptree, lbtree, roart, roart_dcmm, dptree, utree]
OUTPUT_DIR = "./results" # generate output directory 
DISTRIBUTION = "uniform" # choose from [uniform, skew]
THREAD = [40, 30, 20, 10, 5, 1]
TRIAL = 3
N = 100000000
P = 100000000
POOL_NAME = default_pool_name(MODE, TREE) # name of pool file to delete (assume on pmem0), set to "" if running dram index
BINARY_PATH = default_binary_path(MODE, TREE)
OUTPUT_DIR = OUTPUT_DIR + "_" + MODE + '_' + DISTRIBUTION + '/'


# In[33]:


# running script
for op in OPs: 
    benchmark_type = "Benchmark Type: " + op
    command_benchmark_type = "echo {benchmark_type}".format(benchmark_type = benchmark_type)
    os.system(command_benchmark_type)

    output_file = OUTPUT_DIR + define_output_name(op, TREE)
    command_output_file = "echo Output file name: {file}".format(file = output_file)
    os.system(command_output_file)
    
    # create a output directory
    os.system("mkdir {output_dir}".format(output_dir = OUTPUT_DIR))

    command_binary_path = "echo Index binary path: {binary_path}".format(binary_path = BINARY_PATH)
    os.system(command_binary_path)
        
    LD_PRELOAD = "LD_PRELOAD=/usr/lib64/libjemalloc.so"
    
    if DISTRIBUTION == "skew":
        SKEW = "--distribution SELFSIMILAR --skew 0.2"
    else:
        SKEW = ""

    for T in THREAD:
        
        if TREE == "dptree":
            NUMA_COMMAND = "numactl --physcpubind={cpubind} --membind=0".format(cpubind = define_dptree_cpubind(T))
        else:
            NUMA_COMMAND = "numactl --cpunodebind=0 --membind=0"
            
        command = "{numa_command} sudo {ld_preload} ./PiBench {binary_path}             -n {n} -p {p} {op} {skew} --mode time --seconds 10 -t {t} >> {file}".format(numa_command=NUMA_COMMAND,                 ld_preload=LD_PRELOAD, binary_path=BINARY_PATH, n=N, p=P, op=op, skew=SKEW, t=T, file=output_file)

        for i in range(TRIAL):
            print(command)
            os.system("echo {command}".format(command=command))
            os.system("eval {command}".format(command=command))
            os.system("sleep 1")
            if POOL_NAME != "":
                os.system("sudo rm {pool_name}".format(pool_name=POOL_NAME))
            os.system("sleep 3")


# In[ ]:




