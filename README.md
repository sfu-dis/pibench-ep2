# Benchmarking New Persistent Indexes

Here hosts resources for benchmarking newly proposed persistent indexes on Pibench, including benchmark scripts (benchmark.sh & benchmark.py), source code of 6 indexes (see below) and pre-compiled pibench wrappers (wrappers/).

See detailed analysis in our tech report here: https://arxiv.org/abs/2201.13047

# Dependencies
Pibench [https://github.com/sfu-dis/pibench.git] is needed to run any benchmark. <br/>
PMDK [https://pmem.io/pmdk/] is required to run most index under persistent mode. <br/>

# Python benchmark scripts
```
cp -r wrappers /youPathTo_PiBench_Build_Src 
cp benchmark.py /youPathTo_PiBench_Build_Src
python3 benchmark.py
```

# Installation
Pre-compiled shared libs can be found under wrappers directory.<br/>
The README file in each index folder also contains instructions for configuring and producing .so file
1. [DPTree](DPTree/README.md)
2. [FPTree](FP-Tree/README.md)
3. [HOT](Hot/README.md)
4. [LB+-Tree](LB+-Tree/README.md)
5. [Masstree](Masstree/README.md)
6. [ROART](ROART/README.md)
7. [utree](utree/README.md)
