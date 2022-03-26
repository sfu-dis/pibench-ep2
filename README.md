# Benchmarking New Persistent Memory Range Indexes

Here hosts resources for benchmarking newly proposed persistent memory range indexes on Pibench, including benchmark, wrapper generation and results parse scripts (generate_wrappers.sh, benchmark.py, parse.py). Also contains source code of five PM indexes + two DRAM index (see below).

See detailed analysis in our tech report here: https://arxiv.org/abs/2201.13047

# Clone repo and submodule
```
git clone https://github.com/sfu-dis/pibench-ep2.git
cd pibench-ep2
git submodule update --init --recursive
```

# Dependencies
* CMake with VERSION >= 3.14 (pip install cmake --upgrade)
* glibc with VERSION >= 2.34
* [PiBench](https://github.com/sfu-dis/pibench.git): for running all benchmarks. Will be automatically cloned and built if run benchmark.py
* [PMDK](https://pmem.io/pmdk/): required by most indexes for persistent memory management.
* HTM(TSX support) needs to be turned on for FPTree and LB+-Tree. See FPTree `README` for details.


# Building index wrappers for PiBench
To generate pibench wrappers for selected indexes:
```
sudo ./generate_wrappers.sh 
```

or

The `README` in each index directory contains instructions for building each individual PiBench wrapper:


# Benchmarking script
The benchmark script will clone and built pibench for you if it is not found in current folder.

Run it under default settings (Warning! This could take hours to finish!):
```
sudo ./benchmark.py
```

or if wish to run selected experiments for selected indexes with customized parameters:

Tune variables in the script:
```
repeat - # runs for each data point (default 1)

base_size - size of base index before benchmark (default 100M)

seconds - # seconds of operation after load phase (default 10) 

pibench_path - path to PiBench executable (default pibench/build/src/PiBench)

lib_dir - path to PiBench wrappers (default wrappers/)

result_dir - path to folder that stores benchmark results (default ./results)

pool_path - location to place PMem pool (default "" means PMem pool will be created in current dir)

```

There are 6 types of supported experiments, Choose type of experiment and configuations by modifying variables between line 57 ~ 86:
```
Uniform/Skewed/Mixed/Latency/NUMA/VarKey - Specify selected experiments for selected indexes (by default all experiments are selected with all supported indexes)

*_threads - Specify threads for each experiment (default [40,30...1])

*_ops - operation types (default all supported types are included)

self_similar - skew factor for Skewed experiment (default 0.2)

sampling - percentage of samples collected for Latency experiment (default 0.1)

```


# Parse experiment results
By default the experiment results should be saved in ./results folder.

To generate .csv files for all experiments:
```
./parse.py
```

or

modify script to generate .csv for specific experiment (see line 294 and beyond)
