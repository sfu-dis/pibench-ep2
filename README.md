# Benchmarking New Persistent Memory Range Indexes

Here hosts resources for benchmarking newly proposed persistent memory range indexes on Pibench, including benchmark and wrapper generation scripts (generate_wrappers.sh & benchmark.py) and source code of five PM indexes + two DRAM index (see below).

See detailed analysis in our tech report here: https://arxiv.org/abs/2201.13047

# Dependencies
* [PiBench](https://github.com/sfu-dis/pibench.git): for running all benchmarks.
* [PMDK](https://pmem.io/pmdk/): required by most indexes for persistent memory management.

# Building index wrappers for PiBench
The `README` in each index directory contains instructions for building a PiBench wrapper:
1. [DPTree](DPTree/)
2. [FPTree](FP-Tree/)
3. [HOT](Hot/)
4. [LB+-Tree](LB+-Tree/)
5. [Masstree](Masstree/)
6. [ROART](ROART/)
7. [uTree](utree/)

Or configure and run ./generate_wrappers.sh to generate pibench wrappers for selected indexes

# Benchmarking script
See also `benchmark.py`

