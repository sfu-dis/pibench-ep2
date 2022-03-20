#!/usr/bin/env python
# coding: utf-8

# In[30]:


import os
import re
import sys
import csv
from operator import add

result_dir = "results/" # contains all indexes' benchmark results
csv_dir = "datasets/" # where to place generated csv files

csvs = ["Throughput","Latency","MEM_STATS"]

dram_idx = ["FPTree_DRAM","LBTree_DRAM","ROART_DRAM","HOT","Masstree"]
pmem_idx = ["FPTree_PMEM","LBTree_PMEM","ROART_PMDK","ROART_DCMM","DPTree","PACTree"]
vk_idx = ["FPTree_PMEM","LBTree_PMEM","ROART_PMDK","ROART_DCMM","DPTree"]

uniform_ops = ["lookup","insert","update","scan"]
skewed_ops = ["lookup","update","scan"]
vk_ops = ["lookup","insert"]
mixed_ops = ["read_heavy","balanced","write_heavy"]
latency_ops = ["lookup","insert","scan"]

exp_to_headers = {
	'D_Uniform' : ["Threads","fptree_read","fptree_insert","fptree_update","fptree_scan","lbtree_read",
		"lbtree_insert","lbtree_update","lbtree_scan","roart_read","roart_insert","roart_update","roart_scan",
		"hot_read","hot_insert","hot_update","hot_scan","masstree_read","masstree_insert","masstree_update","masstree_scan"],

	'D_Skewed' : ["Threads","fptree_read","fptree_update","fptree_scan","lbtree_read",
		"lbtree_update","lbtree_scan","roart_read","roart_update","roart_scan",
		"hot_read","hot_update","hot_scan","masstree_read","masstree_update","masstree_scan"],

	'P_Uniform' : ["Threads","fptree_read","fptree_insert","fptree_update","fptree_scan","lbtree_read","lbtree_insert",
		"lbtree_update","lbtree_scan","roart_read","roart_insert","roart_update","roart_scan","roart_dcmm_read","roart_dcmm_insert",
		"roart_dcmm_update","roart_dcmm_scan","dptree_read","dptree_insert","dptree_update","dptree_scan","pactree_read",
		"pactree_insert","pactree_update","pactree_scan"],

	'P_Skewed' : ["Threads","fptree_read","fptree_update","fptree_scan","lbtree_read","lbtree_update","lbtree_scan","roart_read",
		"roart_update","roart_scan","roart_dcmm_read","roart_dcmm_update","roart_dcmm_scan","dptree_read","dptree_update",
		"dptree_scan","pactree_read","pactree_update","pactree_scan"],

	'P_VarKey' : ["Threads","fptree_read","fptree_insert","lbtree_read","lbtree_insert","roart_read","roart_insert","roart_dcmm_read",
		"roart_dcmm_insert","dptree_read","dptree_insert"],

	'P_Mixed' : ["Threads","rh_fptree","rh_lbtree","rh_roart","rh_roart_dcmm","rh_dptree","rh_pactree","b_fptree","b_lbtree","b_roart",
		"b_roart_dcmm","b_dptree","b_pactree","wh_fptree","wh_lbtree","wh_roart","wh_roart_dcmm","wh_dptree","wh_pactree"],

	'P_NUMA' : ["Threads","fptree_read","fptree_insert","fptree_update","fptree_scan","lbtree_read","lbtree_insert","lbtree_update",
		"lbtree_scan","roart_read","roart_insert","roart_update","roart_scan","roart_dcmm_read","roart_dcmm_insert","roart_dcmm_update",
		"roart_dcmm_scan","dptree_read","dptree_insert","dptree_update","dptree_scan","pactree_read","pactree_insert","pactree_update",
		"pactree_scan","pactree_numa_read","pactree_numa_insert","pactree_numa_update","pactree_numa_scan"],

	'P_Latency_1' : ["Trees","1t_lookup_min","1t_lookup_50%","1t_lookup_90%","1t_lookup_99%","1t_lookup_99.9%","1t_lookup_99.99%",
		"1t_lookup_99.999%","1t_lookup_max","1t_insert_min","1t_insert_50%","1t_insert_90%","1t_insert_99%","1t_insert_99.9%",
		"1t_insert_99.99%","1t_insert_99.999%","1t_insert_max","1t_scan_min","1t_scan_50%","1t_scan_90%","1t_scan_99%","1t_scan_99.9%",
		"1t_scan_99.99%","1t_scan_99.999%","1t_scan_max"],

	'P_Latency_20' : ["Trees","20t_lookup_min","20t_lookup_50%","20t_lookup_90%","20t_lookup_99%","20t_lookup_99.9%","20t_lookup_99.99%",
		"20t_lookup_99.999%","20t_lookup_max","20t_insert_min","20t_insert_50%","20t_insert_90%","20t_insert_99%","20t_insert_99.9%",
		"20t_insert_99.99%","20t_insert_99.999%","20t_insert_max","20t_scan_min","20t_scan_50%","20t_scan_90%","20t_scan_99%",
		"20t_scan_99.9%","20t_scan_99.99%","20t_scan_99.999%","20t_scan_max"],

	'P_Cache_Miss' : ["Trees","1t_lookup","1t_insert","1t_update","1t_scan","20t_lookup","20t_insert","20t_update","20t_scan"],

	'P_Mem_Stat' : ["Trees","lookup_dram_read","lookup_dram_write","lookup_pmem_read","lookup_pmem_write","insert_dram_read",
		"insert_dram_write","insert_pmem_read","insert_pmem_write","update_dram_read","update_dram_write","update_pmem_read",
		"update_pmem_write","scan_dram_read","scan_dram_write","scan_pmem_read","scan_pmem_write"],
}

exp_to_fname = {
	'D_Uniform' : "dram_indexes_uniform_8b_100m_10s.csv",
	'D_Skewed' : "dram_indexes_sf0.2_8b_100m_10s.csv",
	'P_Uniform' : "pmem_indexes_uniform_8b_100m_10s.csv",
	'P_Skewed' : "pmem_indexes_sf0.2_8b_100m_10s.csv",
	'P_VarKey' : "var_key_pmem.csv",
	'P_Mixed' : "mixed_workload_pmem_uniform_8b_100m_10s.csv",
	'P_NUMA' : "numa_effect_pmem.csv",
	'P_Latency_1' : "tail_latency_pmem_1t.csv",
	'P_Latency_20' : "tail_latency_pmem_20t.csv",
	'P_Cache_Miss' : "cache_misses_pmem.csv",
	'P_Mem_Stat_1' : "pmem_memory_stats_1t_uniform_8b_100m_10s.csv",
	'P_Mem_Stat_20' : "pmem_memory_stats_20t_uniform_8b_100m_10s.csv",
}

tree_to_name = {
	"FPTree_PMEM" : "fptree",
	"LBTree_PMEM" : "lbtree",
	"ROART_PMDK" : "roart",
	"ROART_DCMM" : "roart_dcmm",
	"DPTree" : "dptree",
	"PACTree" : "pactree"
}

if not os.path.exists(csv_dir): 
	os.mkdir(csv_dir);


def write_header(header):
	if os.path.exists(csv_dir + 'temp.csv'):
		os.remove(csv_dir + 'temp.csv')
	f = open(csv_dir + 'temp.csv', 'w')
	writer = csv.writer(f)
	writer.writerow(header)
	return writer

def tp_f(data, args):
	return data

def l_f(data, thread):
	if data[0] == thread:
		return data[1:]
	return 0

def cm_f(data, thread):
	if data[0] == thread:
		return data[2]/data[1]
	return 0

def ms_f(data, thread):
	if data[0] == thread:
		return data[1:]
	return 0


def parse_patterns(lines, p_to_idx, function, args):
	ret = []
	size = len(p_to_idx)
	data = [0] * size

	for line in lines:
		for p in p_to_idx.keys():
			match = re.search(p, line)
			if match:
				data[p_to_idx[p][0]] = int(match.group(p_to_idx[p][1]))
				if p_to_idx[p][0] == size - 1: # last data collected
					num = function(data, args)
					if type(num) is list or num != 0:
						ret.append(num)
						data = [0] * size
				break
	return ret

def throughput(exp, indexes, operations, file_name, header, switch=False):
	writer = write_header(header)

	patterns = {'-t (\d+)' : [0, 1], '- Completed: (\d+)' : [1, 1]}
	th_to_tp = {}
	rows = {}
	if switch:
		indexes, operations = operations, indexes
	for idx in indexes:
		tmp = idx
		for op in operations:
			if switch:
				idx, op = op, tmp
			if exp == 'VarKey' and 'ROART' in idx:
				file = result_dir + idx + '/Uniform/' + idx.lower() + '_' + op + '_results.txt'
			else:
				file = result_dir + idx + '/' + exp + '/' + idx.lower() + '_' + op + '_results.txt'
			if not os.path.exists(file):
				sys.exit("Missing data " + file)
			with open(file) as f:
				lines = f.readlines()
				f.close()
			nums = parse_patterns(lines, patterns, tp_f, None)
			th_to_tp.clear()
			for pair in nums:
				t = pair[0]
				val = pair[1]
				if t in th_to_tp.keys():
					th_to_tp[t] = (th_to_tp[t] + val)/2
				else:
					th_to_tp[t] = val
			for t in list(set(th_to_tp.keys()) | set(rows.keys())):
				if not t in rows.keys():
					rows[t] = [t]
				if not t in th_to_tp.keys():
					rows[t].append('N/A')
				else:
					rows[t].append(int(th_to_tp[t]))
			
	for t in sorted(rows):
		writer.writerow(rows[t])
	os.system("mv " + csv_dir + "temp.csv " + csv_dir + file_name)

def latency(exp, indexes, operations, file_name, header, thread):
	writer = write_header(header)
	content = []
	metric_to_idx = {'max: (\d+)' : [8,1], '99.999%: (\d+)' : [7,1], '99.99%: (\d+)' : [6,1], '99.9%: (\d+)' : [5,1], 
		'99%: (\d+)' : [4,1], '90%: (\d+)' : [3,1], '50%: (\d+)' : [2,1], 'min: (\d+)' : [1,1], '-t (\d+)' : [0, 1]}
	row = 0

	for idx in indexes:
		content.append([tree_to_name[idx]])
		file_dir = result_dir + idx + "/" + exp
		for op in operations:
			file = file_dir + '/' + idx.lower() + '_' + op + '_results.txt'
			if not os.path.exists(file):
				sys.exit("Missing data " + file)
			with open(file) as f:
				lines = f.readlines()
				f.close()
				nums = parse_patterns(lines, metric_to_idx, l_f, thread)
				if len(nums) == 0:
					content[row].extend(['N/A']*8)
				else:
					count = len(nums)
					l = [0] * 8
					for lst in nums:
						l = list(map(add, l, lst))
					l[:] = [x / count for x in l]
					for elt in l:
						content[row].append(int(elt))
		row += 1
	for line in content:
		writer.writerow(line)
	os.system("mv " + csv_dir + "temp.csv " + csv_dir + file_name)

def cache_miss(exp, indexes, operations, file_name, header, threads):
	writer = write_header(header)

	pat_to_idx = {
		'-t (\d+)' : [0, 1],
		'Operations: (\d+)': [1, 1],
		'L3 misses: (\d+)' : [2, 1]
	}

	row = 0
	content = []
	for idx in indexes:
		content.append([tree_to_name[idx]])
		file_dir = result_dir + idx + "/" + exp
		for t in threads:
			for op in operations:
				file = file_dir + '/' + idx.lower() + '_' + op + '_results.txt'
				if not os.path.exists(file):
					sys.exit("Missing data " + file)
				with open(file) as f:
					lines = f.readlines()
					f.close()
					nums = parse_patterns(lines, pat_to_idx, cm_f, t)
					if len(nums) == 0:
						content[row].append('N/A')
					else:
						content[row].append(round(sum(nums)/len(nums), 3))
		row += 1
	for line in content:
		writer.writerow(line)
	os.system("mv " + csv_dir + "temp.csv " + csv_dir + file_name)

def mem_stat(exp, indexes, operations, file_name, header, thread):
	writer = write_header(header)

	pat_to_idx = {
		'-t (\d+)' : [0, 1],
		'DRAM Reads.*: (\d+)' : [1, 1],
		'DRAM Writes.*: (\d+)' : [2, 1],
		'NVM Reads.*: (\d+)' : [3, 1],
		'NVM Writes.*: (\d+)' : [4, 1],
	}

	row = 0
	content = []
	for idx in indexes:
		content.append([tree_to_name[idx]])
		file_dir = result_dir + idx + "/" + exp
		for op in operations:
			file = file_dir + '/' + idx.lower() + '_' + op + '_results.txt'
			if not os.path.exists(file):
				sys.exit("Missing data " + file)
			with open(file) as f:
				lines = f.readlines()
				f.close()
				nums = parse_patterns(lines, pat_to_idx, ms_f, thread)
				if len(nums) == 0:
					content[row].extend(['N/A','N/A','N/A','N/A'])
				else:
					count = len(nums)
					l = [0,0,0,0]
					for lst in nums:
						l = list(map(add, l, lst))
					l[:] = [x / count for x in l]
					for elt in l:
						content[row].append(int(elt))
		row += 1
	for line in content:
		writer.writerow(line)
	os.system("mv " + csv_dir + "temp.csv " + csv_dir + file_name)

if "Throughput" in csvs:
	# DRAM Uniform
	throughput('Uniform', dram_idx, uniform_ops, exp_to_fname["D_Uniform"], exp_to_headers["D_Uniform"])

	# DRAM Skewed
	throughput('Skewed', dram_idx, skewed_ops, exp_to_fname["D_Skewed"], exp_to_headers["D_Skewed"])

	# PMEM Uniform
	throughput('Uniform', pmem_idx, uniform_ops, exp_to_fname["P_Uniform"], exp_to_headers["P_Uniform"])

	# PMEM Skewed
	throughput('Skewed', pmem_idx, skewed_ops, exp_to_fname["P_Skewed"], exp_to_headers["P_Skewed"])

	# PMEM Mixed
	throughput('Mixed', pmem_idx, mixed_ops, exp_to_fname["P_Mixed"], exp_to_headers["P_Mixed"], True)

	# PMEM VarKey
	throughput('VarKey', vk_idx, vk_ops, exp_to_fname["P_VarKey"], exp_to_headers["P_VarKey"])

	# PMEM NUMA
	throughput('NUMA', pmem_idx, uniform_ops, exp_to_fname["P_NUMA"], exp_to_headers["P_NUMA"])


if "Latency" in csvs:
	# PMEM Latency 1 thread
	latency('Latency', pmem_idx, latency_ops, exp_to_fname["P_Latency_1"], exp_to_headers['P_Latency_1'], 1)

	# PMEM Latency 20 thread
	latency('Latency', pmem_idx, latency_ops, exp_to_fname["P_Latency_20"], exp_to_headers['P_Latency_20'], 20)


if "MEM_STATS" in csvs:
	# cache miss
	cache_miss('Uniform', pmem_idx, uniform_ops, exp_to_fname['P_Cache_Miss'], exp_to_headers['P_Cache_Miss'], [1, 20])

	# Memory Stat 1t
	mem_stat('Uniform', pmem_idx, uniform_ops, exp_to_fname['P_Mem_Stat_1'], exp_to_headers['P_Mem_Stat'], 1)

	# Memory Stat 20t
	mem_stat('Uniform', pmem_idx, uniform_ops, exp_to_fname['P_Mem_Stat_20'], exp_to_headers['P_Mem_Stat'], 20)
