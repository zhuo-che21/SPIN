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
where `i` represent the traffic mode, can be choosed from [uniform_random, tornado, bit_complement, bit_reverse, bit_rotation, neighbor, shuffle, transpose]
and `j` represent the injection rate. 

And for deterministic routing algorithm, you can run
```bash
./build/NULL/gem5.opt \
configs/example/garnet_synth_traffic.py \
--network=garnet --num-cpus=64 --num-dirs=64 \
--topology=Mesh_XYZ --routing-algorithm=2 --mesh-rows=4 --mesh-heights=4 \
--inj-vnet=0 --synthetic=`i` \
--sim-cycles=10000 --injectionrate=`j` 
```

Also you could run 
```bash
./build/NULL/gem5.opt configs/example/project.py
```
directly after build the architecture, which is an integrated command files to get all datas used in our analysis on ``gem5/network_stats_project.txt''. (It takes long time)