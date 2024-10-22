Increasing demand for computer power has led to the proliferation of Interconnection Networks in data centers, 
supercomputers, and computing chips. Among that, the performance of a Network of Chips (NoC) is greatly dependent on the topology.

In this project, we implement a 3D torus and a 3D mesh topology with fully adaptive deadlock-free routing
algorithm respectively, and compare the performance with 2D and 3D deterministic routing algorithms over
the torus and mesh topology respectively, in some different traffic modes.

For the mesh part, you should run 
```bash
scons build/NULL/gem5.opt PROTOCOL=Garnet_standalone -j $(nproc)
```
to build the mesh architecture after replacing the src and configs folders. 

To run our 3D adaptive routing algorithm, you can run 
```bash
./build/NULL/gem5.opt \
configs/example/garnet_synth_traffic.py \
--network=garnet --num-cpus=64 --num-dirs=64 \
--topology=Mesh_XYZ --routing-algorithm=3 --mesh-rows=4 --mesh-heights=4 \
--inj-vnet=0 --synthetic=`i` \
--sim-cycles=10000 --injectionrate=`j` 
```
where `i` represent the traffic mode, can be chosen from [uniform_random, tornado, bit_complement, bit_reverse, bit_rotation, neighbor, shuffle, transpose]
and `j` represents the injection rate. 

For a deterministic routing algorithm, you can run
```bash
./build/NULL/gem5.opt \
configs/example/garnet_synth_traffic.py \
--network=garnet --num-cpus=64 --num-dirs=64 \
--topology=Mesh_XYZ --routing-algorithm=2 --mesh-rows=4 --mesh-heights=4 \
--inj-vnet=0 --synthetic=`i` \
--sim-cycles=10000 --injectionrate=`j` 
```

Also, you could run 
```bash
./build/NULL/gem5.opt configs/example/project.py
```
directly after building the architecture, which is an integrated command file to get all data used in our analysis on ``gem5/network_stats_project.txt''. (It takes a long time)
