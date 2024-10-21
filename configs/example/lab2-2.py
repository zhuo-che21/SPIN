import os, argparse, sys

for i in [32,48,64,128]:
    os.system("sed -i '$a\ i=%s' network_stats_2-3.txt" % (i))
    for inj in [0.05,0.20,0.37,0.5]:
        os.system("./build/NULL/gem5.opt \
        configs/example/garnet_synth_traffic.py \
        --network=garnet --num-cpus=64 --num-dirs=64 --link-width-bits=%s\
        --topology=Mesh_XY --mesh-rows=8 \
        --inj-vnet=0 --synthetic=uniform_random \
        --sim-cycles=10000 --injectionrate=%s" % (i, inj))
        os.system("grep 'average_packet_latency' m5out/stats.txt | sed 's/system.ruby.network.average_packet_latency\s*/average_packet_latency = /' >> network_stats_2-3.txt")
        # os.system("grep 'average_hops' m5out/stats.txt | sed 's/system.ruby.network.average_hops\s*/average_hops = /' >> network_stats_2-3.txt")
        os.system("grep 'average_packet_queueing_latency' m5out/stats.txt | sed 's/system.ruby.network.average_packet_queueing_latency\s*/average_packet_queueing_latency = /' >> network_stats_2-3.txt")
        os.system("grep 'average_packet_network_latency' m5out/stats.txt | sed 's/system.ruby.network.average_packet_network_latency\s*/average_packet_network_latency = /' >> network_stats_2-3.txt")
        # os.system("grep 'success_rate' m5out/stats.txt | sed 's/system.ruby.network.success_rate\s*/success_rate = /' >> network_stats_2-3.txt")